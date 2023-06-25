![Piper logo](etc/logo.png)

A fast, local neural text to speech system that sounds great and is optimized for the Raspberry Pi 4.
Piper is used in a [variety of projects](#people-using-piper).

``` sh
echo 'Welcome to the world of speech synthesis!' | \
  ./piper --model en-us-blizzard_lessac-medium.onnx --output_file welcome.wav
```

[Listen to voice samples](https://rhasspy.github.io/piper-samples) and check out a [video tutorial by Thorsten Müller](https://youtu.be/rjq5eZoWWSo)

[![Sponsored by Nabu Casa](etc/nabu_casa_sponsored.png)](https://nabucasa.com)

Voices are trained with [VITS](https://github.com/jaywalnut310/vits/) and exported to the [onnxruntime](https://onnxruntime.ai/).

## Voices

Our goal is to support Home Assistant and the [Year of Voice](https://www.home-assistant.io/blog/2022/12/20/year-of-voice/).

[Download voices](https://github.com/rhasspy/piper/releases/tag/v0.0.2) for the supported languages:

* Catalan (ca)
* Danish (da)
* German (de)
* British English (en-gb)
* U.S. English (en-us)
* Spanish (es)
* Finnish (fi)
* French (fr)
* Greek (el-gr)
* Icelandic (is)
* Italian (it)
* Kazakh (kk)
* Nepali (ne)
* Dutch (nl)
* Norwegian (no)
* Polish (pl)
* Brazilian Portuguese (pt-br)
* Russian (ru)
* Swedish (sv-se)
* Ukrainian (uk)
* Vietnamese (vi)
* Chinese (zh-cn)


## Installation

Download a release:

* [amd64](https://github.com/rhasspy/piper/releases/download/v1.0.0/piper_amd64.tar.gz) (64-bit desktop Linux)
* [arm64](https://github.com/rhasspy/piper/releases/download/v1.0.0/piper_arm64.tar.gz) (64-bit Raspberry Pi 4)
* [armv7](https://github.com/rhasspy/piper/releases/download/v1.0.0/piper_armv7.tar.gz) (32-bit Raspberry Pi 3/4)

If you want to build from source, see the [Makefile](Makefile) and [C++ source](src/cpp).
You must download and extract [piper-phonemize](https://github.com/rhasspy/piper-phonemize) to `lib/Linux-$(uname -m)/piper_phonemize` before building.
For example, `lib/Linux-x86_64/piper_phonemize/lib/libpiper_phonemize.so` should exist for AMD/Intel machines (as well as everything else from `libpiper_phonemize-amd64.tar.gz`).


## Usage

1. [Download a voice](#voices) and extract the `.onnx` and `.onnx.json` files
2. Run the `piper` binary with text on standard input, `--model /path/to/your-voice.onnx`, and `--output_file output.wav`

For example:

``` sh
echo 'Welcome to the world of speech synthesis!' | \
  ./piper --model en-us-lessac-medium.onnx --output_file welcome.wav
```

For multi-speaker models, use `--speaker <number>` to change speakers (default: 0).

See `piper --help` for more options.


## People using Piper

Piper has been used in the following projects/papers:

* [Home Assistant](https://github.com/home-assistant/addons/blob/master/piper/README.md)
* [Rhasspy 3](https://github.com/rhasspy/rhasspy3/)
* [NVDA - NonVisual Desktop Access](https://www.nvaccess.org/post/in-process-8th-may-2023/#voices)
* [Image Captioning for the Visually Impaired and Blind: A Recipe for Low-Resource Languages](https://www.techrxiv.org/articles/preprint/Image_Captioning_for_the_Visually_Impaired_and_Blind_A_Recipe_for_Low-Resource_Languages/22133894)
* [Open Voice Operating System](https://github.com/OpenVoiceOS/ovos-tts-plugin-piper)
* [JetsonGPT](https://github.com/shahizat/jetsonGPT)


## Training

See the [training guide](TRAINING.md) and the [source code](src/python).

Pretrained checkpoints are available on [Hugging Face](https://huggingface.co/datasets/rhasspy/piper-checkpoints/tree/main)


## Running in Python

See [src/python_run](src/python_run)

Run `scripts/setup.sh` to create a virtual environment and install the requirements. Then run:

``` sh
echo 'Welcome to the world of speech synthesis!' | scripts/piper \
  --model /path/to/voice.onnx \
  --output_file welcome.wav
```

If you'd like to use a GPU, install the `onnxruntime-gpu` package:


``` sh
.venv/bin/pip3 install onnxruntime-gpu
```

and then run `scripts/piper` with the `--cuda` argument. You will need to have a functioning CUDA environment, such as what's available in [NVIDIA's PyTorch containers](https://catalog.ngc.nvidia.com/orgs/nvidia/containers/pytorch).

