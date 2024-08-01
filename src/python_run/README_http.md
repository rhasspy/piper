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

Alternatively, you can run this process in a docker container.

Build the image with:

```sh
docker build . -f Dockerfile_http -t piper-http
```

Run the web server:

```sh
docker run -p 5000:5000 -v /path/to/data-dir:/home/piper/data piper-http --model ...
```

where _/path/to/data-dir_ is the local folder where you store data files.

Using a `GET` request:

```sh
curl -G --data-urlencode 'text=This is a test.' -o test.wav 'localhost:5000'
```

Using a `POST` request:

```sh
curl -X POST -H 'Content-Type: text/plain' --data 'This is a test.' -o test.wav 'localhost:5000'
```
