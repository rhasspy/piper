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
