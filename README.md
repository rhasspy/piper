![Piper logo](etc/logo.png)

A fast, local neural text to speech system that sounds great and is optimized for the Raspberry Pi 4.
Piper is used in a [variety of projects](#people-using-piper).

``` sh
echo 'Welcome to the world of speech synthesis!' | \
  ./piper --model en_US-lessac-medium.onnx --output_file welcome.wav
```

[Listen to voice samples](https://rhasspy.github.io/piper-samples) and check out a [video tutorial by Thorsten Müller](https://youtu.be/rjq5eZoWWSo)

Voices are trained with [VITS](https://github.com/jaywalnut310/vits/) and exported to the [onnxruntime](https://onnxruntime.ai/).

This is a project of the [Open Home Foundation](https://www.openhomefoundation.org/).

## Voices

Our goal is to support Home Assistant and the [Year of Voice](https://www.home-assistant.io/blog/2022/12/20/year-of-voice/).

[Download voices](VOICES.md) for the supported languages:

* Arabic (ar_JO)
* Catalan (ca_ES)
* Czech (cs_CZ)
* Welsh (cy_GB)
* Danish (da_DK)
* German (de_DE)
* Greek (el_GR)
* English (en_GB, en_US)
* Spanish (es_ES, es_MX)
* Finnish (fi_FI)
* French (fr_FR)
* Hungarian (hu_HU)
* Icelandic (is_IS)
* Italian (it_IT)
* Georgian (ka_GE)
* Kazakh (kk_KZ)
* Luxembourgish (lb_LU)
* Nepali (ne_NP)
* Dutch (nl_BE, nl_NL)
* Norwegian (no_NO)
* Polish (pl_PL)
* Portuguese (pt_BR, pt_PT)
* Romanian (ro_RO)
* Russian (ru_RU)
* Serbian (sr_RS)
* Swedish (sv_SE)
* Swahili (sw_CD)
* Turkish (tr_TR)
* Ukrainian (uk_UA)
* Vietnamese (vi_VN)
* Chinese (zh_CN)

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

### **1. Installation**

First, install Piper using `pip`:

```sh
pip install piper-tts
```

### **2. Running Piper**

To generate speech from text, use the following command:

```sh
echo 'Welcome to the world of speech synthesis!' | piper \
  --model en_US-lessac-medium \
  --output_file welcome.wav
```

The first time you use a model, Piper will automatically download the required [voice files](https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0).  

To **control where voice models are stored**, specify `--data-dir` and `--download-dir`:

```sh
piper --model en_US-lessac-medium --output_file welcome.wav \
  --data-dir /path/to/data --download-dir /path/to/downloads
```

### **3. Using a GPU (Optional)**

If you want to **enable GPU acceleration**, install the `onnxruntime-gpu` package:

```sh
pip install onnxruntime-gpu
```

Then, run Piper with GPU support:

```sh
piper --model en_US-lessac-medium --output_file welcome.wav --cuda
```

**Note:**  
* A properly configured CUDA environment is required.  
* If you're using **NVIDIA GPUs**, you can set up CUDA using [NVIDIA's PyTorch containers](https://catalog.ngc.nvidia.com/orgs/nvidia/containers/pytorch).

---

### **4. Running Piper in a Python Script**

If you prefer to use Piper directly in Python, follow these steps:

#### **Step 1: Install Dependencies**

Ensure you have Python 3.7+ installed.  
(Optional but recommended) Create a virtual environment:

```sh
# Create and activate a virtual environment
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate

# Install Piper
pip install piper-tts
```

#### **Step 2: Download a Voice Model**

Download a voice model before running Piper:

```sh
mkdir -p models/en_US
wget -O models/en_US/en_US-medium.onnx https://huggingface.co/rhasspy/piper/resolve/main/en/en_US-medium.onnx
```

#### **Step 3: Generate Speech in Python**

Now, you can use Piper in Python:

```python
from piper import PiperTTS

# Load the model
tts = PiperTTS(model_path="models/en_US/en_US-medium.onnx")

# Generate speech from text
audio = tts.synthesize("Hello, I am Piper, your text-to-speech assistant.")

# Save the audio output
with open("output.wav", "wb") as f:
    f.write(audio)

print("Speech synthesis complete! Check output.wav")
```

#### **Step 4: Play the Audio**

Once the file `output.wav` is generated, play it with:

```sh
ffplay output.wav  # Linux/Mac
start output.wav   # Windows
```
