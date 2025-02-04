# Piper HTTP Server

Install the requirements into your virtual environment:

```sh
.venv/bin/pip3 install -r requirements_http.txt
```

Run the web server:

```sh
.venv/bin/python3 -m piper.http_server --model ...
```

See `--help` for more options.

Using a `GET` request:

```sh
curl -G --data-urlencode 'text=This is a test.' -o test.wav 'localhost:5000'
```

Using a `POST` request:

```sh
curl -X POST -H 'Content-Type: text/plain' --data 'This is a test.' -o test.wav 'localhost:5000'
```

## Troubleshooting Installation Issues on macOS

If you encounter the following error while installing Piper TTS on macOS:

```
error: Distribution `piper-phonemize==1.1.0 @ registry+https://pypi.org/simple` can't be installed because it doesn't have a source distribution or wheel for the current platform
```

This issue occurs because `piper-phonemize` does not provide a compatible distribution for macOS. To resolve this, you can use an alternative package that supports macOS:

### Solution: Use `piper-phonemize-cross`

1. Install `piper-phonemize-cross`, which includes prebuilt wheels for macOS:
   ```sh
   pip install piper-phonemize-cross
   ```

2. Install `piper-tts` without dependencies to avoid conflicts:
   ```sh
   pip install piper-tts --no-deps
   ```

### Alternative Solution: Build from Source

If you prefer, you can manually build `piper-phonemize` from source:

```sh
git clone https://github.com/rhasspy/piper.git
cd piper/phonemize
pip install .
```

For more details, refer to the official Piper TTS repository: [Piper GitHub](https://github.com/rhasspy/piper).

