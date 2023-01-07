#!/usr/bin/env python3
import argparse
import logging
from pathlib import Path
from typing import Optional

import torch

from .vits.lightning import VitsModel

_LOGGER = logging.getLogger("larynx_train.export_torchscript")


def main():
    """Main entry point"""
    torch.manual_seed(1234)

    parser = argparse.ArgumentParser()
    parser.add_argument("checkpoint", help="Path to model checkpoint (.ckpt)")
    parser.add_argument("output", help="Path to output model (.onnx)")

    parser.add_argument(
        "--debug", action="store_true", help="Print DEBUG messages to the console"
    )
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    _LOGGER.debug(args)

    # -------------------------------------------------------------------------

    args.checkpoint = Path(args.checkpoint)
    args.output = Path(args.output)
    args.output.parent.mkdir(parents=True, exist_ok=True)

    model = VitsModel.load_from_checkpoint(args.checkpoint)
    model_g = model.model_g

    num_symbols = model_g.n_vocab
    num_speakers = model_g.n_speakers

    # Inference only
    model_g.eval()

    with torch.no_grad():
        model_g.dec.remove_weight_norm()

    model_g.forward = model_g.infer

    jitted_model = torch.jit.script(model_g)
    torch.jit.save(jitted_model, str(args.output))

    _LOGGER.info("Saved TorchScript model to %s", args.output)


# -----------------------------------------------------------------------------

if __name__ == "__main__":
    main()
