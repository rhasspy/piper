import argparse
import json
import logging
from pathlib import Path

import torch
from pytorch_lightning import Trainer
from pytorch_lightning.callbacks import ModelCheckpoint, EarlyStopping, LearningRateMonitor

from .vits.lightning import VitsModel

_LOGGER = logging.getLogger(__package__)


def main():
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger("fsspec").setLevel(logging.WARNING)
    logging.getLogger("matplotlib").setLevel(logging.WARNING)
    logging.getLogger("PIL").setLevel(logging.WARNING)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--dataset-dir",
        type=str,
        required=True,
        help="Path to pre-processed dataset directory"
    )
    parser.add_argument(
        "--checkpoint-epochs",
        type=int,
        help="Save checkpoint every N epochs (default: 1)",
    )
    parser.add_argument(
        "--quality",
        default="medium",
        choices=("x-low", "medium", "high"),
        help="Quality/size of model (default: medium)",
    )
    parser.add_argument(
        "--resume_from_single_speaker_checkpoint",
        help="For multi-speaker models only. Converts a single-speaker checkpoint to multi-speaker and resumes training",
    )
    VitsModel.add_model_specific_args(parser)
    parser.add_argument(
        "--accelerator",
        type=str,
    )
    parser.add_argument(
        "--devices",
        type=int,
    )
    parser.add_argument(
        "--log_every_n_steps",
        type=int,
    )
    parser.add_argument(
        "--max_epochs",
        type=int,
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=1234
    )
    parser.add_argument(
        "--random_seed",
        type=bool,
        default=False
    )
    parser.add_argument(
        "--resume_from_checkpoint",
        type=str,
    )
    parser.add_argument(
        "--precision",
        type=str,
    )
    parser.add_argument(
        "--num_ckpt",
        type=int,
        default=1,
        help="# of ckpts saved."
    )
    parser.add_argument(
        "--default_root_dir",
        type=str,
        help="Default root dir for checkpoints and logs."
    )
    parser.add_argument(
        "--save_last",
        type=bool,
        default=None,
        help="Always save the last checkpoint."
    )
    parser.add_argument(
        "--monitor",
        type=str,
        default="val_loss",
        help="Metric to monitor."
    )
    parser.add_argument(
        "--monitor_mode",
        type=str,
        default="min",
        help="Mode to monitor."
    )
    parser.add_argument(
        "--early_stop_patience",
        type=int,
        default=0,
        help="Number of validation cycles to allow to pass without improvement before stopping training"
    )
    args = parser.parse_args()
    _LOGGER.debug(args)

    args.dataset_dir = Path(args.dataset_dir)
    if not args.default_root_dir:
        args.default_root_dir = args.dataset_dir

    torch.backends.cudnn.benchmark = True

    if args.random_seed:
        seed = torch.seed()
        _LOGGER.debug("Using random seed: %s", seed)
    else:
        torch.manual_seed(args.seed)
        _LOGGER.debug("Using manual seed: %s", args.seed)
    
    # Function to check if the GPU supports Tensor Cores
    def supports_tensor_cores():
        # Assuming that Tensor Cores are supported if the compute capability is 7.0 or higher
        # This is a simplification; you might need a more detailed check based on your specific requirements
        return torch.cuda.get_device_capability(0)[0] >= 7

    # Set the float32 matrix multiplication precision based on GPU support for Tensor Cores
    if supports_tensor_cores():
        # Set to 'high' or 'medium' based on your preference
        torch.set_float32_matmul_precision('high')

    config_path = args.dataset_dir / "config.json"
    dataset_path = args.dataset_dir / "dataset.jsonl"

    with open(config_path, "r", encoding="utf-8") as config_file:
        # See preprocess.py for format
        config = json.load(config_file)
        num_symbols = int(config["num_symbols"])
        num_speakers = int(config["num_speakers"])
        sample_rate = int(config["audio"]["sample_rate"])

    # List of argument names to include
    allowed_args = [
        "accelerator",
        "devices",
        "log_every_n_steps",
        "max_epochs",
        "precision",
        "default_root_dir",
    ]

    # Filter the arguments
    filtered_args = {key: value for key, value in vars(args).items() if key in allowed_args}

    # Initialize callbacks
    callbacks = []

    if args.checkpoint_epochs is not None:
        checkpoint_callback = ModelCheckpoint(
            every_n_epochs=args.checkpoint_epochs,
            save_top_k=args.num_ckpt,
            monitor=args.monitor,
            mode=args.monitor_mode,
            save_last=args.save_last
        )
        callbacks.append(checkpoint_callback)
        _LOGGER.debug(
            "Checkpoints will be saved every %s epoch(s)", args.checkpoint_epochs
        )
        _LOGGER.debug(
            "%s Checkpoints will be saved", args.num_ckpt
        )

    if args.early_stop_patience > 0:
        # Early stopping callback
        early_stopping_callback = EarlyStopping(
            monitor='val_loss',
            patience=args.early_stop_patience,
            verbose=True,
            mode='min'
        )
        callbacks.append(early_stopping_callback)

    # Learning rate monitor callback
    lr_monitor_callback = LearningRateMonitor(logging_interval='epoch')
    callbacks.append(lr_monitor_callback)

    trainer = Trainer(**filtered_args, callbacks=callbacks)

    dict_args = vars(args)
    if args.quality == "x-low":
        dict_args["hidden_channels"] = 96
        dict_args["inter_channels"] = 96
        dict_args["filter_channels"] = 384
    elif args.quality == "high":
        dict_args["resblock"] = "1"
        dict_args["resblock_kernel_sizes"] = (3, 7, 11)
        dict_args["resblock_dilation_sizes"] = (
            (1, 3, 5),
            (1, 3, 5),
            (1, 3, 5),
        )
        dict_args["upsample_rates"] = (8, 8, 2, 2)
        dict_args["upsample_initial_channel"] = 512
        dict_args["upsample_kernel_sizes"] = (16, 16, 4, 4)

    model = VitsModel(
        num_symbols=num_symbols,
        num_speakers=num_speakers,
        sample_rate=sample_rate,
        dataset=[dataset_path],
        **dict_args,
    )

    if args.resume_from_single_speaker_checkpoint:
        assert (
            num_speakers > 1
        ), "--resume_from_single_speaker_checkpoint is only for multi-speaker models. Use --resume_from_checkpoint for single-speaker models."

        # Load single-speaker checkpoint
        _LOGGER.debug(
            "Resuming from single-speaker checkpoint: %s",
            args.resume_from_single_speaker_checkpoint,
        )
        model_single = VitsModel.load_from_checkpoint(
            args.resume_from_single_speaker_checkpoint,
            dataset=None,
        )
        g_dict = model_single.model_g.state_dict()
        for key in list(g_dict.keys()):
            # Remove keys that can't be copied over due to missing speaker embedding
            if (
                key.startswith("dec.cond")
                or key.startswith("dp.cond")
                or ("enc.cond_layer" in key)
            ):
                g_dict.pop(key, None)

        # Copy over the multi-speaker model, excluding keys related to the
        # speaker embedding (which is missing from the single-speaker model).
        load_state_dict(model.model_g, g_dict)
        load_state_dict(model.model_d, model_single.model_d.state_dict())
        _LOGGER.info(
            "Successfully converted single-speaker checkpoint to multi-speaker"
        )

    trainer.fit(model, ckpt_path=args.resume_from_checkpoint)


def load_state_dict(model, saved_state_dict):
    state_dict = model.state_dict()
    new_state_dict = {}

    for k, v in state_dict.items():
        if k in saved_state_dict:
            # Use saved value
            new_state_dict[k] = saved_state_dict[k]
        else:
            # Use initialized value
            _LOGGER.debug("%s is not in the checkpoint", k)
            new_state_dict[k] = v

    model.load_state_dict(new_state_dict)


# -----------------------------------------------------------------------------


if __name__ == "__main__":
    main()
