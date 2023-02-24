import argparse
import json
import logging
from pathlib import Path

import torch
from pytorch_lightning import Trainer
from pytorch_lightning.callbacks import ModelCheckpoint

from .vits.lightning import VitsModel

_LOGGER = logging.getLogger(__package__)


def main():
    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--dataset-dir", required=True, help="Path to pre-processed dataset directory"
    )
    parser.add_argument(
        "--checkpoint-epochs",
        type=int,
        help="Save checkpoint every N epochs (default: 1)",
    )
    Trainer.add_argparse_args(parser)
    VitsModel.add_model_specific_args(parser)
    parser.add_argument("--seed", type=int, default=1234)
    args = parser.parse_args()
    _LOGGER.debug(args)

    args.dataset_dir = Path(args.dataset_dir)
    if not args.default_root_dir:
        args.default_root_dir = args.dataset_dir

    torch.backends.cudnn.benchmark = True
    torch.manual_seed(args.seed)

    config_path = args.dataset_dir / "config.json"
    dataset_path = args.dataset_dir / "dataset.jsonl"

    with open(config_path, "r", encoding="utf-8") as config_file:
        # See preprocess.py for format
        config = json.load(config_file)
        num_symbols = int(config["num_symbols"])
        num_speakers = int(config["num_speakers"])
        sample_rate = int(config["audio"]["sample_rate"])

    trainer = Trainer.from_argparse_args(args)
    if args.checkpoint_epochs is not None:
        trainer.callbacks = [ModelCheckpoint(every_n_epochs=args.checkpoint_epochs)]
        _LOGGER.debug(
            "Checkpoints will be saved every %s epoch(s)", args.checkpoint_epochs
        )

    dict_args = vars(args)
    model = VitsModel(
        num_symbols=num_symbols,
        num_speakers=num_speakers,
        sample_rate=sample_rate,
        dataset=[dataset_path],
        **dict_args
    )

    trainer.fit(model)


# -----------------------------------------------------------------------------


if __name__ == "__main__":
    main()
