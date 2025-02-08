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

## Making Requests

The server supports both GET and POST requests. Synthesis parameters can be modified through query parameters in either type of request.

### Using GET Requests

Basic example:
```sh
curl -G --data-urlencode 'text=This is a test.' -o test.wav 'localhost:5000'
```

With synthesis parameters:
```sh
curl -G \
  --data-urlencode 'text=This is a test.' \
  --data-urlencode 'speaker_id=1' \
  --data-urlencode 'length_scale=1.2' \
  --data-urlencode 'noise_scale=0.6' \
  -o test.wav \
  'localhost:5000'
```

### Using POST Requests

Basic example:
```sh
curl -X POST \
  -H 'Content-Type: text/plain' \
  --data 'This is a test.' \
  -o test.wav \
  'localhost:5000'
```

With synthesis parameters:
```sh
curl -X POST \
  -H 'Content-Type: text/plain' \
  --data 'This is a test.' \
  -o test.wav \
  'localhost:5000?speaker_id=1&length_scale=1.2&noise_scale=0.6'
```

## Synthesis Parameters

The following parameters can be modified through request query parameters:

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| speaker_id | int | ID of the speaker to use | 0 |
| length_scale | float | Phoneme length scaling factor | Model default |
| noise_scale | float | Generator noise scaling factor | Model default |
| noise_w | float | Phoneme width noise | Model default |
| sentence_silence | float | Seconds of silence after each sentence | 0.0 |

If a parameter isn't provided in the request, it will fall back to the command-line default value. If neither is provided, the model's default values will be used.
