#!/bin/bash

inotifywait -q -m -e close_write welcome.wav |
	while read -r filename event; do
		aplay welcome.wav # or "./$filename"
	done
