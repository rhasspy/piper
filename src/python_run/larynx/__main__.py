import argparse
import logging
import sys
import time
from functools import partial
from pathlib import Path

from . import Larynx

_FILE = Path(__file__)
_DIR = _FILE.parent
_LOGGER = logging.getLogger(_FILE.stem)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model", required=True, help="Path to Onnx model file")
    parser.add_argument("-c", "--config", help="Path to model config file")
    parser.add_argument(
        "-f", "--output_file", help="Path to output WAV file (default: stdout)"
    )
    parser.add_argument(
        "-d", "--output_dir", help="Path to output directory (default: cwd)"
    )
    parser.add_argument("-s", "--speaker", type=int, help="Id of speaker (default: 0)")
    parser.add_argument("--noise-scale", type=float, help="Generator noise")
    parser.add_argument("--length-scale", type=float, help="Phoneme length")
    parser.add_argument("--noise-w", type=float, help="Phoneme width noise")
    parser.add_argument("--cuda", action="store_true", help="Use GPU")
    #
    parser.add_argument(
        "--debug", action="store_true", help="Print DEBUG messages to console"
    )
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO)

    voice = Larynx(args.model, config_path=args.config, use_cuda=args.cuda)
    synthesize = partial(
        voice.synthesize,
        speaker_id=args.speaker,
        length_scale=args.length_scale,
        noise_scale=args.noise_scale,
        noise_w=args.noise_w,
    )

    if args.output_dir:
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Read line-by-line
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue

            wav_bytes = synthesize(line)
            wav_path = output_dir / f"{time.monotonic_ns()}.wav"
            wav_path.write_bytes(wav_bytes)
            _LOGGER.info("Wrote %s", wav_path)
    else:
        # Read entire input
        text = sys.stdin.read()
        wav_bytes = synthesize(text)

        if (not args.output_file) or (args.output_file == "-"):
            # Write to stdout
            sys.stdout.buffer.write(wav_bytes)
        else:
            with open(args.output_file, "wb") as output_file:
                output_file.write(wav_bytes)


if __name__ == "__main__":
    main()
