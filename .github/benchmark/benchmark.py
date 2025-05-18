#!/usr/bin/env python3
import os
import time
import wave
from packaging.version import Version
import onnxruntime as ort

print('onnxruntime version: ' + str(ort.__version__))

ort_version = Version(ort.__version__)

if ort_version > Version('1.10'):
    print(ort.get_build_info())

# verify execution providers
providers = ort.get_available_providers()

print(f'execution providers:  {providers}')

from piper import PiperVoice
from piper.download import ensure_voice_exists, find_voice, get_voices

DEFAULT_PROMPT = """A rainbow is a meteorological phenomenon that is caused by reflection, refraction and dispersion of light in water droplets resulting in a spectrum of light appearing in the sky.
It takes the form of a multi-colored circular arc.
Rainbows caused by sunlight always appear in the section of sky directly opposite the Sun.
With tenure, Suzie’d have all the more leisure for yachting, but her publications are no good.
Shaw, those twelve beige hooks are joined if I patch a young, gooey mouth.
Are those shy Eurasian footwear, cowboy chaps, or jolly earthmoving headgear?
The beige hue on the waters of the loch impressed all, including the French queen, before she heard that symphony again, just as young Arthur wanted.
"""


def main(model='en_US-lessac-high', config=None, cache=os.environ.get('PIPER_CACHE'),
         speaker=0, length_scale=1.0, noise_scale=0.667, noise_w=0.8, sentence_silence=0.2,
         prompt=DEFAULT_PROMPT, output='/dev/null', backend='cpu', runs=5, dump=False, **kwargs):
    # Download voice info
    try:
        voices_info = get_voices(cache, update_voices=True)
    except Exception as error:
        print(f"Failed to download Piper voice list  ({error})")
        voices_info = get_voices(cache)

    # Resolve aliases for backwards compatibility with old voice names
    aliases_info = {}
    for voice_info in voices_info.values():
        for voice_alias in voice_info.get("aliases", []):
            aliases_info[voice_alias] = {"_is_alias": True, **voice_info}

    voices_info.update(aliases_info)

    if not os.path.isfile(os.path.join(cache, model)):
        model_name = model
        ensure_voice_exists(model, cache, cache, voices_info)
        model, config = find_voice(model, [cache])
    else:
        model_name = os.path.splitext(os.path.basename(model))[0]

    # Load model
    if backend == 'cpu':
        providers = ['CPUExecutionProvider']
        use_cuda = False
    elif backend == 'cuda':
        providers = ['CUDAExecutionProvider', 'CPUExecutionProvider']
        use_cuda = True
    elif backend == 'tensorrt':
        # Typically you want to include CUDA as a fallback if TensorRT fails.
        providers = ['TensorrtExecutionProvider', 'CUDAExecutionProvider', 'CPUExecutionProvider']
        use_cuda = True
    else:
        raise ValueError(f"Unknown backend '{backend}'")
    print(f"Loading {model} with backend={backend} providers={providers}")
    voice = PiperVoice.load(model, config_path=config, use_cuda=use_cuda)

    # get the speaker name->ID mapping
    speaker_id_map = voices_info[model_name]['speaker_id_map']

    if not speaker_id_map:
        speaker_id_map = {'Default': 0}

    # get the inverse speakerID->name mapping
    speaker_id_inv = {}

    for key, value in speaker_id_map.items():
        speaker_id_inv[value] = key

    # optional mode to dump all speakers
    if dump:
        speakers = list(speaker_id_map.values())
        runs = 1
        output_dir = output
        os.makedirs(output_dir, exist_ok=True)
    else:
        speakers = [speaker]

    for speaker in speakers:
        synthesize_args = {
            "speaker_id": speaker,
            "length_scale": length_scale,
            "noise_scale": noise_scale,
            "noise_w": noise_w,
            "sentence_silence": sentence_silence,
        }

        # Run benchmarking iterations
        for run in range(runs):
            if dump:
                output = os.path.join(output_dir, f"{model_name}_{speaker:04d}_{speaker_id_inv[speaker]}.wav")

            with wave.open(output, "wb") as wav_file:
                wav_file.setnchannels(1)
                start = time.perf_counter()
                voice.synthesize(prompt, wav_file, **synthesize_args)
                end = time.perf_counter()

                inference_duration = end - start

                frames = wav_file.getnframes()
                rate = wav_file.getframerate()
                audio_duration = frames / float(rate)

            print(f"Piper TTS model:    {model_name}")
            print(f"Output saved to:    {output}")
            print(f"Inference duration: {inference_duration:.3f} sec")
            print(f"Audio duration:     {audio_duration:.3f} sec")
            print(f"Realtime factor:    {inference_duration / audio_duration:.3f}")
            print(f"Inverse RTF (RTFX): {audio_duration / inference_duration:.3f}\n")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--model', type=str, default='en_US-lessac-high', help="model path or name to download")
    parser.add_argument('--config', type=str, default=None,
                        help="path to the model's json config (if unspecified, will be inferred from --model)")
    parser.add_argument('--cache', type=str, default=os.environ.get('PIPER_CACHE'),
                        help="the location to save downloaded models")

    parser.add_argument('--speaker', type=int, default=0, help="the speaker ID from the voice to use")
    parser.add_argument('--length-scale', type=float, default=1.0, help="speaking speed")
    parser.add_argument('--noise-scale', type=float, default=0.667, help="noise added to the generator")
    parser.add_argument('--noise-w', type=float, default=0.8, help="phoneme width variation")
    parser.add_argument('--sentence-silence', type=float, default=0.2, help="seconds of silence after each sentence")

    parser.add_argument('--prompt', type=str, default=None,
                        help="the test prompt to generate (will be set to a default prompt if left none)")
    parser.add_argument('--output', type=str, default=None,
                        help="path to output audio wav file to save (will be /data/tts/piper-$MODEL.wav by default)")
    parser.add_argument('--runs', type=int, default=5, help="the number of benchmarking iterations to run")
    parser.add_argument('--dump', action='store_true', help="dump all speaker voices to the output directory")
    parser.add_argument('--disable-cuda', action='store_false', dest='use_cuda',
                        help="disable CUDA and use CPU for inference instead")
    parser.add_argument('--verbose', action='store_true', help="enable onnxruntime debug logging")

    args = parser.parse_args()

    if args.verbose:
        ort.set_default_logger_severity(0)

    if not args.prompt:
        args.prompt = DEFAULT_PROMPT

    if not args.output:
        args.output = f"/data/audio/tts/piper-{os.path.splitext(os.path.basename(args.model))[0]}.wav"

    print(args)

    main(**vars(args))