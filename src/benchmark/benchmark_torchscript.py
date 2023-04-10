#!/usr/bin/env python3
import argparse
import json
import time
import sys

import torch

_NOISE_SCALE = 0.667
_LENGTH_SCALE = 1.0
_NOISE_W = 0.8


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-m", "--model", required=True, help="Path to Torchscript file (.ts)"
    )
    parser.add_argument("-c", "--config", help="Path to model config file (.json)")
    args = parser.parse_args()

    if not args.config:
        args.config = f"{args.model}.json"

    with open(args.config, "r", encoding="utf-8") as config_file:
        config = json.load(config_file)

    sample_rate = config["audio"]["sample_rate"]
    utterances = [json.loads(line) for line in sys.stdin]

    start_time = time.monotonic_ns()
    model = torch.jit.load(args.model)
    end_time = time.monotonic_ns()

    model.eval()

    load_sec = (end_time - start_time) / 1e9
    synthesize_rtf = []
    for utterance in utterances:
        phoneme_ids = utterance["phoneme_ids"]
        speaker_id = utterance.get("speaker_id")
        synthesize_rtf.append(
            synthesize(
                model,
                phoneme_ids,
                speaker_id,
                sample_rate,
            )
        )

    json.dump(
        {"load_sec": load_sec, "synthesize_rtf": synthesize_rtf},
        sys.stdout,
    )


def synthesize(model, phoneme_ids, speaker_id, sample_rate) -> float:
    text = torch.LongTensor(phoneme_ids).unsqueeze(0)
    text_lengths = torch.LongTensor([len(phoneme_ids)])
    sid = torch.LongTensor([speaker_id]) if speaker_id is not None else None

    start_time = time.monotonic_ns()
    audio = (
        model(
            text,
            text_lengths,
            sid,
            torch.FloatTensor([_NOISE_SCALE]),
            torch.FloatTensor([_LENGTH_SCALE]),
            torch.FloatTensor([_NOISE_W]),
        )[0]
        .detach()
        .numpy()
        .squeeze()
    )
    end_time = time.monotonic_ns()

    audio_sec = (len(audio) / 2) / sample_rate
    infer_sec = (end_time - start_time) / 1e9

    return infer_sec / audio_sec


if __name__ == "__main__":
    main()
