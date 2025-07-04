![Piper logo](etc/logo.png)

A fast, local neural text to speech system that sounds great and is optimized for the Raspberry Pi 4.
Piper is used in a [variety of projects](#people-using-piper).

``` sh
echo 'Welcome to the world of speech synthesis!' | \
  ./piper --model en_US-lessac-medium.onnx --output_file welcome.wav
```

[Listen to voice samples](https://rhasspy.github.io/piper-samples) and check out a [video tutorial by Thorsten Müller](https://youtu.be/rjq5eZoWWSo)

Voices are trained with [VITS](https://github.com/jaywalnut310/vits/) and exported to the [onnxruntime](https://onnxruntime.ai/).

[![A library from the Open Home Foundation](https://www.openhomefoundation.org/badges/ohf-library.png)](https://www.openhomefoundation.org/)

## Voices

Our goal is to support Home Assistant and the [Year of Voice](https://www.home-assistant.io/blog/2022/12/20/year-of-voice/).

[Download voices](VOICES.md) for the supported languages:

* العربية, Jordan (Arabic, ar_JO)
* Català, Spain (Catalan, ca_ES)
* Čeština, Czech Republic (Czech, cs_CZ)
* Cymraeg, Great Britain (Welsh, cy_GB)
* Dansk, Denmark (Danish, da_DK)
* Deutsch, Germany (German, de_DE)
* Ελληνικά, Greece (Greek, el_GR)
* English, Great Britain (English, en_GB)
* English, United States (English, en_US)
* Español, Argentina (Spanish, es_AR)
* Español, Spain (Spanish, es_ES)
* Español, Mexico (Spanish, es_MX)
* فارسی, Iran (Farsi, fa_IR)
* Suomi, Finland (Finnish, fi_FI)
* Français, France (French, fr_FR)
* Magyar, Hungary (Hungarian, hu_HU)
* íslenska, Iceland (Icelandic, is_IS)
* Italiano, Italy (Italian, it_IT)
* ქართული ენა, Georgia (Georgian, ka_GE)
* қазақша, Kazakhstan (Kazakh, kk_KZ)
* Lëtzebuergesch, Luxembourg (Luxembourgish, lb_LU)
* Latviešu, Latvia (Latvian, lv_LV)
* മലയാളം, India (Malayalam, ml_IN)
* हिंदी, India (Hindi, hi_IN)
* नेपाली, Nepal (Nepali, ne_NP)
* Nederlands, Belgium (Dutch, nl_BE)
* Nederlands, Netherlands (Dutch, nl_NL)
* Norsk, Norway (Norwegian, no_NO)
* Polski, Poland (Polish, pl_PL)
* Português, Brazil (Portuguese, pt_BR)
* Português, Portugal (Portuguese, pt_PT)
* Română, Romania (Romanian, ro_RO)
* Русский, Russia (Russian, ru_RU)
* Slovenčina, Slovakia (Slovak, sk_SK)
* Slovenščina, Slovenia (Slovenian, sl_SI)
* srpski, Serbia (Serbian, sr_RS)
* Svenska, Sweden (Swedish, sv_SE)
* Kiswahili, Democratic Republic of the Congo (Swahili, sw_CD)
* Türkçe, Turkey (Turkish, tr_TR)
* украї́нська мо́ва, Ukraine (Ukrainian, uk_UA)
* Tiếng Việt, Vietnam (Vietnamese, vi_VN)
* 简体中文, China (Chinese, zh_CN)

You will need two files per voice:

1. A `.onnx` model file, such as [`en_US-lessac-medium.onnx`](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/lessac/medium/en_US-lessac-medium.onnx)
2. A `.onnx.json` config file, such as [`en_US-lessac-medium.onnx.json`](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json)

The `MODEL_CARD` file for each voice contains important licensing information. Piper is intended for text to speech research, and does not impose any additional restrictions on voice models. Some voices may have restrictive licenses, however, so please review them carefully!


## Installation

You can [run Piper with Python](#running-in-python) or download a binary release:

* [amd64](https://github.com/rhasspy/piper/releases/download/v1.2.0/piper_amd64.tar.gz) (64-bit desktop Linux)
* [arm64](https://github.com/rhasspy/piper/releases/download/v1.2.0/piper_arm64.tar.gz) (64-bit Raspberry Pi 4)
* [armv7](https://github.com/rhasspy/piper/releases/download/v1.2.0/piper_armv7.tar.gz) (32-bit Raspberry Pi 3/4)

If you want to build from source, see the [Makefile](Makefile) and [C++ source](src/cpp).
You must download and extract [piper-phonemize](https://github.com/rhasspy/piper-phonemize) to `lib/Linux-$(uname -m)/piper_phonemize` before building.
For example, `lib/Linux-x86_64/piper_phonemize/lib/libpiper_phonemize.so` should exist for AMD/Intel machines (as well as everything else from `libpiper_phonemize-amd64.tar.gz`).


## Usage

1. [Download a voice](#voices) and extract the `.onnx` and `.onnx.json` files
2. Run the `piper` binary with text on standard input, `--model /path/to/your-voice.onnx`, and `--output_file output.wav`

For example:

``` sh
echo 'Welcome to the world of speech synthesis!' | \
  ./piper --model en_US-lessac-medium.onnx --output_file welcome.wav
```

For multi-speaker models, use `--speaker <number>` to change speakers (default: 0).

See `piper --help` for more options.

### Streaming Audio

Piper can stream raw audio to stdout as its produced:

``` sh
echo 'This sentence is spoken first. This sentence is synthesized while the first sentence is spoken.' | \
  ./piper --model en_US-lessac-medium.onnx --output-raw | \
  aplay -r 22050 -f S16_LE -t raw -
```

This is **raw** audio and not a WAV file, so make sure your audio player is set to play 16-bit mono PCM samples at the correct sample rate for the voice.

### JSON Input

The `piper` executable can accept JSON input when using the `--json-input` flag. Each line of input must be a JSON object with `text` field. For example:

``` json
{ "text": "First sentence to speak." }
{ "text": "Second sentence to speak." }
```

Optional fields include:

* `speaker` - string
    * Name of the speaker to use from `speaker_id_map` in config (multi-speaker voices only)
* `speaker_id` - number
    * Id of speaker to use from 0 to number of speakers - 1 (multi-speaker voices only, overrides "speaker")
* `output_file` - string
    * Path to output WAV file
    
The following example writes two sentences with different speakers to different files:

``` json
{ "text": "First speaker.", "speaker_id": 0, "output_file": "/tmp/speaker_0.wav" }
{ "text": "Second speaker.", "speaker_id": 1, "output_file": "/tmp/speaker_1.wav" }
```


## People using Piper

Piper has been used in the following projects/papers:

* [POTaTOS](https://www.youtube.com/watch?v=Dz95q6XYjwY)
* [Home Assistant](https://github.com/home-assistant/addons/blob/master/piper/README.md)
* [Rhasspy 3](https://github.com/rhasspy/rhasspy3/)
* [NVDA - NonVisual Desktop Access](https://www.nvaccess.org/post/in-process-8th-may-2023/#voices)
* [Image Captioning for the Visually Impaired and Blind: A Recipe for Low-Resource Languages](https://www.techrxiv.org/articles/preprint/Image_Captioning_for_the_Visually_Impaired_and_Blind_A_Recipe_for_Low-Resource_Languages/22133894)
* [Open Voice Operating System](https://github.com/OpenVoiceOS/ovos-tts-plugin-piper)
* [JetsonGPT](https://github.com/shahizat/jetsonGPT)
* [LocalAI](https://github.com/go-skynet/LocalAI)
* [Lernstick EDU / EXAM: reading clipboard content aloud with language detection](https://lernstick.ch/)
* [Natural Speech - A plugin for Runelite, an OSRS Client](https://github.com/phyce/rl-natural-speech)
* [mintPiper](https://github.com/evuraan/mintPiper)
* [Vim-Piper](https://github.com/wolandark/vim-piper)

## Training

See the [training guide](TRAINING.md) and the [source code](src/python).

Pretrained checkpoints are available on [Hugging Face](https://huggingface.co/datasets/rhasspy/piper-checkpoints/tree/main)


## Running in Python

See [src/python_run](src/python_run)

Install with `pip`:

``` sh
pip install piper-tts
```

and then run:

``` sh
echo 'Welcome to the world of speech synthesis!' | piper \
  --model en_US-lessac-medium \
  --output_file welcome.wav
```

This will automatically download [voice files](https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0) the first time they're used. Use `--data-dir` and `--download-dir` to adjust where voices are found/downloaded.

If you'd like to use a GPU, install the `onnxruntime-gpu` package:


``` sh
.venv/bin/pip3 install onnxruntime-gpu
```

and then run `piper` with the `--cuda` argument. You will need to have a functioning CUDA environment, such as what's available in [NVIDIA's PyTorch containers](https://catalog.ngc.nvidia.com/orgs/nvidia/containers/pytorch).
