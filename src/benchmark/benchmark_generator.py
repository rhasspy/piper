#!/usr/bin/env python3
import argparse
import logging
import json
import time
import statistics
import sys

import torch

_LOGGER = logging.getLogger(__name__)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-m", "--model", required=True, help="Path to generator file (.pt)"
    )
    parser.add_argument("-c", "--config", help="Path to model config file (.json)")
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG)

    if not args.config:
        args.config = f"{args.model}.json"

    with open(args.config, "r", encoding="utf-8") as config_file:
        config = json.load(config_file)

    sample_rate = config["audio"]["sample_rate"]
    utterances = [json.loads(line) for line in sys.stdin]

    start_time = time.monotonic_ns()
    model = torch.load(args.model)
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
        {
            "load_sec": load_sec,
            "rtf_mean": statistics.mean(synthesize_rtf),
            "rtf_stdev": statistics.stdev(synthesize_rtf),
            "synthesize_rtf": synthesize_rtf,
        },
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
        )[0]
        .detach()
        .numpy()
        .squeeze()
    )
    end_time = time.monotonic_ns()

    audio_sec = len(audio) / sample_rate
    infer_sec = (end_time - start_time) / 1e9
    rtf = infer_sec / audio_sec

    _LOGGER.debug(
        "Real-time factor: %s (infer=%s sec, audio=%s sec)",
        rtf,
        infer_sec,
        audio_sec,
    )

    return rtf


if __name__ == "__main__":
    main()
