import logging
from pathlib import Path
from typing import List, Optional, Tuple, Union

import pytorch_lightning as pl
import torch
from torch import autocast
from torch.nn import functional as F
from torch.utils.data import DataLoader, Dataset, random_split
import matplotlib.pyplot as plt

from .commons import slice_segments
from .dataset import Batch, PiperDataset, UtteranceCollate
from .losses import discriminator_loss, feature_loss, generator_loss, kl_loss
from .mel_processing import mel_spectrogram_torch, spec_to_mel_torch
from .models import MultiPeriodDiscriminator, SynthesizerTrn

_LOGGER = logging.getLogger("vits.lightning")


class VitsModel(pl.LightningModule):
    def __init__(
        self,
        num_symbols: int,
        num_speakers: int,
        # audio
        resblock="2",
        resblock_kernel_sizes=(3, 5, 7),
        resblock_dilation_sizes=(
            (1, 2),
            (2, 6),
            (3, 12),
        ),
        upsample_rates=(8, 8, 4),
        upsample_initial_channel=256,
        upsample_kernel_sizes=(16, 16, 8),
        # mel
        filter_length: int = 1024,
        hop_length: int = 256,
        win_length: int = 1024,
        mel_channels: int = 80,
        sample_rate: int = 22050,
        sample_bytes: int = 2,
        channels: int = 1,
        mel_fmin: float = 0.0,
        mel_fmax: Optional[float] = None,
        # model
        inter_channels: int = 192,
        hidden_channels: int = 192,
        filter_channels: int = 768,
        n_heads: int = 2,
        n_layers: int = 6,
        kernel_size: int = 3,
        p_dropout: float = 0.1,
        n_layers_q: int = 3,
        use_spectral_norm: bool = False,
        gin_channels: int = 0,
        use_sdp: bool = True,
        segment_size: int = 8192,
        # training
        dataset: Optional[List[Union[str, Path]]] = None,
        learning_rate: float = 2e-4,
        override_learning_rate: bool = False,
        weight_decay: float = 1e-2,
        betas: Tuple[float, float] = (0.8, 0.99),
        eps: float = 1e-9,
        batch_size: int = 1,
        lr_decay: float = 0.999875,
        lr_reduce_enabled: bool = False,
        lr_reduce_patience: int = 10,
        lr_reduce_factor: float = 0.5,
        init_lr_ratio: float = 1.0,
        warmup_epochs: int = 0,
        c_mel: int = 45,
        c_kl: float = 1.0,
        grad_clip: Optional[float] = None,
        num_workers: int = 1,
        seed: int = 1234,
        num_test_examples: int = 5,
        validation_split: float = 0.1,
        max_phoneme_ids: Optional[int] = None,
        show_plot = False,
        **kwargs,
    ):
        super().__init__()
        self.save_hyperparameters()
        self.automatic_optimization = False  # Disable automatic optimization

        if (self.hparams.num_speakers > 1) and (self.hparams.gin_channels <= 0):
            # Default gin_channels for multi-speaker model
            self.hparams.gin_channels = 512

        # Set up models
        self.model_g = SynthesizerTrn(
            n_vocab=self.hparams.num_symbols,
            spec_channels=self.hparams.filter_length // 2 + 1,
            segment_size=self.hparams.segment_size // self.hparams.hop_length,
            inter_channels=self.hparams.inter_channels,
            hidden_channels=self.hparams.hidden_channels,
            filter_channels=self.hparams.filter_channels,
            n_heads=self.hparams.n_heads,
            n_layers=self.hparams.n_layers,
            kernel_size=self.hparams.kernel_size,
            p_dropout=self.hparams.p_dropout,
            resblock=self.hparams.resblock,
            resblock_kernel_sizes=self.hparams.resblock_kernel_sizes,
            resblock_dilation_sizes=self.hparams.resblock_dilation_sizes,
            upsample_rates=self.hparams.upsample_rates,
            upsample_initial_channel=self.hparams.upsample_initial_channel,
            upsample_kernel_sizes=self.hparams.upsample_kernel_sizes,
            n_speakers=self.hparams.num_speakers,
            gin_channels=self.hparams.gin_channels,
            use_sdp=self.hparams.use_sdp,
        )
        self.model_d = MultiPeriodDiscriminator(
            use_spectral_norm=self.hparams.use_spectral_norm
        )

        # Dataset splits
        self._train_dataset: Optional[Dataset] = None
        self._val_dataset: Optional[Dataset] = None
        self._test_dataset: Optional[Dataset] = None
        self._load_datasets(validation_split, num_test_examples, max_phoneme_ids)

        # State kept between training optimizers
        self._y = None
        self._y_hat = None

        if self.hparams.show_plot or self.hparams.plot_save_path:
            # Initialize plot
            self.fig, self.ax = plt.subplots()
            self.ax2 = None
            self.gen_losses = []
            self.disc_losses = []
            self.val_losses = []
            self.epochs = []
            self.gen_lrs = []
            self.disc_lrs = []

    def _load_datasets(
        self,
        validation_split: float,
        num_test_examples: int,
        max_phoneme_ids: Optional[int] = None,
    ):
        if self.hparams.dataset is None:
            _LOGGER.debug("No dataset to load")
            return

        full_dataset = PiperDataset(
            self.hparams.dataset, max_phoneme_ids=max_phoneme_ids
        )
        valid_set_size = int(len(full_dataset) * validation_split)
        train_set_size = len(full_dataset) - valid_set_size - num_test_examples

        self._train_dataset, self._test_dataset, self._val_dataset = random_split(
            full_dataset, [train_set_size, num_test_examples, valid_set_size]
        )

    def forward(self, text, text_lengths, scales, sid=None):
        noise_scale = scales[0]
        length_scale = scales[1]
        noise_scale_w = scales[2]
        audio, *_ = self.model_g.infer(
            text,
            text_lengths,
            noise_scale=noise_scale,
            length_scale=length_scale,
            noise_scale_w=noise_scale_w,
            sid=sid,
        )

        return audio

    def train_dataloader(self):
        return DataLoader(
            self._train_dataset,
            collate_fn=UtteranceCollate(
                is_multispeaker=self.hparams.num_speakers > 1,
                segment_size=self.hparams.segment_size,
            ),
            num_workers=self.hparams.num_workers,
            batch_size=self.hparams.batch_size,
        )

    def val_dataloader(self):
        return DataLoader(
            self._val_dataset,
            collate_fn=UtteranceCollate(
                is_multispeaker=self.hparams.num_speakers > 1,
                segment_size=self.hparams.segment_size,
            ),
            num_workers=self.hparams.num_workers,
            batch_size=self.hparams.batch_size,
        )

    def test_dataloader(self):
        return DataLoader(
            self._test_dataset,
            collate_fn=UtteranceCollate(
                is_multispeaker=self.hparams.num_speakers > 1,
                segment_size=self.hparams.segment_size,
            ),
            num_workers=self.hparams.num_workers,
            batch_size=self.hparams.batch_size,
        )

    def training_step(self, batch: Batch, batch_idx: int):
        # Manually access optimizers
        opt_g, opt_d = self.optimizers()

        if self.first_epoch:
            if self.hparams.override_learning_rate:
                _LOGGER.info("First epoch, overriding learning rate to %f", self.hparams.learning_rate)
                for param_group in opt_g.param_groups:
                    param_group['lr'] = self.hparams.learning_rate
                for param_group in opt_d.param_groups:
                    param_group['lr'] = self.hparams.learning_rate
                self.first_epoch = False


        # Perform generator step
        loss_gen_all = self.training_step_g(batch)
        opt_g.zero_grad()
        self.manual_backward(loss_gen_all)

         # Gradient clipping for generator
        if self.hparams.grad_clip is not None:
            torch.nn.utils.clip_grad_norm_(self.model_g.parameters(), self.hparams.grad_clip)

        opt_g.step()

        # Perform discriminator step
        loss_disc_all = self.training_step_d(batch)
        opt_d.zero_grad()
        self.manual_backward(loss_disc_all)

        # Gradient clipping for discriminator
        if self.hparams.grad_clip is not None:
            torch.nn.utils.clip_grad_norm_(self.model_d.parameters(), self.hparams.grad_clip)

        opt_d.step()

        # Log learning rates
        self.log("gen_lr", opt_g.param_groups[0]['lr'])
        self.log("disc_lr", opt_d.param_groups[0]['lr'])
        self.log("step", self.global_step, prog_bar=True)

        return {"loss_gen": loss_gen_all, "loss_disc": loss_disc_all}

    def training_step_g(self, batch: Batch):
        x, x_lengths, y, _, spec, spec_lengths, speaker_ids = (
            batch.phoneme_ids,
            batch.phoneme_lengths,
            batch.audios,
            batch.audio_lengths,
            batch.spectrograms,
            batch.spectrogram_lengths,
            batch.speaker_ids if batch.speaker_ids is not None else None,
        )
        (
            y_hat,
            l_length,
            _attn,
            ids_slice,
            _x_mask,
            z_mask,
            (_z, z_p, m_p, logs_p, _m_q, logs_q),
        ) = self.model_g(x, x_lengths, spec, spec_lengths, speaker_ids)
        self._y_hat = y_hat

        mel = spec_to_mel_torch(
            spec,
            self.hparams.filter_length,
            self.hparams.mel_channels,
            self.hparams.sample_rate,
            self.hparams.mel_fmin,
            self.hparams.mel_fmax,
        )
        y_mel = slice_segments(
            mel,
            ids_slice,
            self.hparams.segment_size // self.hparams.hop_length,
        )
        y_hat_mel = mel_spectrogram_torch(
            y_hat.squeeze(1),
            self.hparams.filter_length,
            self.hparams.mel_channels,
            self.hparams.sample_rate,
            self.hparams.hop_length,
            self.hparams.win_length,
            self.hparams.mel_fmin,
            self.hparams.mel_fmax,
        )
        y = slice_segments(
            y,
            ids_slice * self.hparams.hop_length,
            self.hparams.segment_size,
        )  # slice

        # Save for training_step_d
        self._y = y

        _y_d_hat_r, y_d_hat_g, fmap_r, fmap_g = self.model_d(y, y_hat)

        with autocast(self.device.type, enabled=False):
            # Generator loss
            loss_dur = torch.sum(l_length.float())
            loss_mel = F.l1_loss(y_mel, y_hat_mel) * self.hparams.c_mel
            loss_kl = kl_loss(z_p, logs_q, m_p, logs_p, z_mask) * self.hparams.c_kl

            loss_fm = feature_loss(fmap_r, fmap_g)
            loss_gen, _losses_gen = generator_loss(y_d_hat_g)
            loss_gen_all = loss_gen + loss_fm + loss_mel + loss_dur + loss_kl

            self.log("gen_loss", loss_gen_all)

            return loss_gen_all

    def training_step_d(self, batch: Batch):
        # From training_step_g
        y = self._y
        y_hat = self._y_hat
        y_d_hat_r, y_d_hat_g, _, _ = self.model_d(y, y_hat.detach())

        with autocast(self.device.type, enabled=False):
            # Discriminator
            loss_disc, _losses_disc_r, _losses_disc_g = discriminator_loss(
                y_d_hat_r, y_d_hat_g
            )
            loss_disc_all = loss_disc

            self.log("disc_loss", loss_disc_all)

            return loss_disc_all

    def validation_step(self, batch: Batch, batch_idx: int):
        val_loss = self.training_step_g(batch) + self.training_step_d(batch)

        self.log("val_loss", val_loss)

        # # Generate audio examples
        # for utt_idx, test_utt in enumerate(self._test_dataset):
        #     text = test_utt.phoneme_ids.unsqueeze(0).to(self.device)
        #     text_lengths = torch.LongTensor([len(test_utt.phoneme_ids)]).to(self.device)
        #     scales = [0.667, 1.0, 0.8]
        #     sid = (
        #         test_utt.speaker_id.to(self.device)
        #         if test_utt.speaker_id is not None
        #         else None
        #     )
        #     test_audio = self(text, text_lengths, scales, sid=sid).detach()

        #     # Scale to make louder in [-1, 1]
        #     test_audio = test_audio * (1.0 / max(0.01, abs(test_audio.max())))

        #     tag = test_utt.text or str(utt_idx)
        #     self.logger.experiment.add_audio(
        #         tag, 
        #         test_audio,
        #         self.global_step,
        #         sample_rate=self.hparams.sample_rate
        #     )

        if self.hparams.lr_reduce_enabled:
            # Step the scheduler with the validation loss
            scheduler_g, scheduler_d = self.lr_schedulers()
            scheduler_g.step(val_loss)
            scheduler_d.step(val_loss)

        return val_loss
    
    def on_train_epoch_end(self):
        if not self.hparams.show_plot and not self.hparams.plot_save_path:
            return

        avg_gen_loss = self.trainer.callback_metrics.get("gen_loss")
        avg_disc_loss = self.trainer.callback_metrics.get("disc_loss")

        avg_gen_loss_cpu = avg_gen_loss.detach().cpu() if avg_gen_loss.is_cuda else avg_gen_loss.detach()
        self.gen_losses.append(avg_gen_loss_cpu)

        avg_disc_loss_cpu = avg_disc_loss.detach().cpu() if avg_disc_loss.is_cuda else avg_disc_loss.detach()
        self.disc_losses.append(avg_disc_loss_cpu)

        # Capture validation loss
        val_loss = self.trainer.callback_metrics.get("val_loss")
        val_loss_cpu = val_loss.detach().cpu() if val_loss.is_cuda else val_loss.detach()
        self.val_losses.append(val_loss_cpu)


        # Capture learning rate
        gen_lr = self.trainer.callback_metrics.get("gen_lr")
        disc_lr = self.trainer.callback_metrics.get("disc_lr")

        gen_lr_cpu = gen_lr.detach().cpu() if gen_lr.is_cuda else gen_lr.detach()
        disc_lr_cpu = disc_lr.detach().cpu() if disc_lr.is_cuda else disc_lr.detach()

        self.gen_lrs.append(gen_lr_cpu)
        self.disc_lrs.append(disc_lr_cpu)

        # Update epochs for plot
        self.epochs.append(self.current_epoch)

        # Update plot
        self.update_plot()

    def on_train_start(self):
        self.first_epoch = True

    def configure_optimizers(self):
        optimizers = [
            torch.optim.AdamW(
                self.model_g.parameters(),
                lr=self.hparams.learning_rate,
                betas=self.hparams.betas,
                eps=self.hparams.eps,
                weight_decay=self.hparams.weight_decay,
            ),
            torch.optim.AdamW(
                self.model_d.parameters(),
                lr=self.hparams.learning_rate,
                betas=self.hparams.betas,
                eps=self.hparams.eps,
                weight_decay=self.hparams.weight_decay,
            ),
        ]


        if self.hparams.lr_reduce_enabled:
            schedulers = [
                torch.optim.lr_scheduler.ReduceLROnPlateau(
                    optimizers[0], mode='min', factor=self.hparams.lr_reduce_factor, patience=self.hparams.lr_reduce_patience
                ),
                torch.optim.lr_scheduler.ReduceLROnPlateau(
                    optimizers[1], mode='min', factor=self.hparams.lr_reduce_factor, patience=self.hparams.lr_reduce_patience
                ),
            ]
        else:
            schedulers = [
                torch.optim.lr_scheduler.ExponentialLR(
                    optimizers[0], gamma=self.hparams.lr_decay
                ),
                torch.optim.lr_scheduler.ExponentialLR(
                    optimizers[1], gamma=self.hparams.lr_decay
                )
            ]

        return optimizers, schedulers

    def update_plot(self):
        self.ax.clear()

        self.ax.plot(self.epochs, self.gen_losses, label='Generator Loss', color='tab:blue')
        self.ax.plot(self.epochs, self.disc_losses, label='Discriminator Loss', color='tab:orange')
        self.ax.plot(self.epochs, self.val_losses, label='Validation Loss', color='tab:green')
        self.ax.set_xlabel('Epoch')
        self.ax.set_ylabel('Loss')

        # Create a secondary y-axis for the learning rate
        if self.ax2 is not None:
            self.ax2.clear()
        self.ax2 = self.ax.twinx()
        self.ax2.plot(self.epochs, self.gen_lrs, label='Generator Learning Rate', color='tab:red')
        self.ax2.plot(self.epochs, self.disc_lrs, label='Discriminator Learning Rate', color='tab:purple')
        self.ax2.set_xlabel('Epoch')
        self.ax2.set_ylabel('Learning Rate')

        self.ax.legend(loc='upper left')
        self.ax2.legend(loc='upper right')

        title = F'Training Progress - Epoch: {self.current_epoch}'
        self.ax.set_title(title)
        self.ax.get_figure().canvas.manager.set_window_title(title)
        self.ax.grid(True)

        if self.hparams.show_plot:
            plt.draw()
            plt.pause(0.01)

        if self.hparams.plot_save_path:
            self.ax.get_figure().savefig(self.hparams.plot_save_path)

    @staticmethod
    def add_model_specific_args(parent_parser):
        parser = parent_parser.add_argument_group("VitsModel")
        parser.add_argument("--batch-size", type=int, required=True)
        parser.add_argument("--validation-split", type=float, default=0.1)
        parser.add_argument("--num-test-examples", type=int, default=5)
        parser.add_argument(
            "--max-phoneme-ids",
            type=int,
            help="Exclude utterances with phoneme id lists longer than this",
        )
        #
        parser.add_argument("--hidden-channels", type=int, default=192)
        parser.add_argument("--inter-channels", type=int, default=192)
        parser.add_argument("--filter-channels", type=int, default=768)
        parser.add_argument("--n-layers", type=int, default=6)
        parser.add_argument("--n-heads", type=int, default=2)

        parser.add_argument("--lr-decay", type=float, default=0.999875)
        parser.add_argument("--lr-reduce-enabled", type=bool, default=False)
        parser.add_argument("--lr-reduce-factor", type=float, default=0.5)
        parser.add_argument("--lr-reduce-patience", type=int, default=10)
        
        parser.add_argument("--show-plot", type=bool, default=False)
        parser.add_argument("--plot-save-path", type=str, default=None)

        parser.add_argument("--learning-rate", type=float, default=2e-4)
        parser.add_argument("--weight-decay", type=float, default=1e-2)
        parser.add_argument("--override-learning-rate", type=bool, default=False)
        parser.add_argument("--grad-clip", type=float, default=None)

        return parent_parser
