#!/bin/bash

inotifywait -q -m -e close_write output.wav |
	while read -r filename event; do
		aplay output.wav # or "./$filename"
	done
