#!/usr/bin/env python3
import argparse
import json
import time
import sys

import onnxruntime
import numpy as np

_NOISE_SCALE = 0.667
_LENGTH_SCALE = 1.0
_NOISE_W = 0.8
_SPEAKER_ID = 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model", required=True, help="Path to Onnx model file")
    parser.add_argument("-c", "--config", help="Path to model config file")
    args = parser.parse_args()

    if not args.config:
        args.config = f"{args.model}.json"

    with open(args.config, "r", encoding="utf-8") as config_file:
        config = json.load(config_file)

    sample_rate = config["audio"]["sample_rate"]
    utterances = [json.loads(line) for line in sys.stdin]

    start_time = time.monotonic_ns()
    session = onnxruntime.InferenceSession(args.model)
    end_time = time.monotonic_ns()

    load_sec = (end_time - start_time) / 1e9
    synthesize_rtf = []
    for utterance in utterances:
        phoneme_ids = utterance["phoneme_ids"]
        speaker_id = utterance.get("speaker_id")
        synthesize_rtf.append(
            synthesize(
                session,
                phoneme_ids,
                speaker_id,
                sample_rate,
            )
        )

    json.dump(
        {"load_sec": load_sec, "synthesize_rtf": synthesize_rtf},
        sys.stdout,
    )


def synthesize(session, phoneme_ids, speaker_id, sample_rate) -> float:
    phoneme_ids_array = np.expand_dims(np.array(phoneme_ids, dtype=np.int64), 0)
    phoneme_ids_lengths = np.array([phoneme_ids_array.shape[1]], dtype=np.int64)
    scales = np.array(
        [_NOISE_SCALE, _LENGTH_SCALE, _NOISE_W],
        dtype=np.float32,
    )

    sid = None

    if speaker_id is not None:
        sid = np.array([speaker_id], dtype=np.int64)

    # Synthesize through Onnx
    start_time = time.monotonic_ns()
    audio = session.run(
        None,
        {
            "input": phoneme_ids_array,
            "input_lengths": phoneme_ids_lengths,
            "scales": scales,
            "sid": sid,
        },
    )[0].squeeze()
    end_time = time.monotonic_ns()

    audio_sec = (len(audio) / 2) / sample_rate
    infer_sec = (end_time - start_time) / 1e9

    return infer_sec / audio_sec


if __name__ == "__main__":
    main()
