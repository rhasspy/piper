import argparse
import sys

from . import Larynx


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model", help="Path to Onnx model file")
    parser.add_argument("--cuda", action="store_true", help="Use GPU")
    args = parser.parse_args()

    voice = Larynx(args.model, use_cuda=args.cuda)
    wav_bytes = voice.synthesize(sys.stdin.read())
    sys.stdout.buffer.write(wav_bytes)


if __name__ == "__main__":
    main()
