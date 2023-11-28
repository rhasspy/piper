#!/bin/bash

if pgrep -x piper >/dev/null; then
	echo "Found piper! Stopping..."
	pkill -INT piper
	pkill playnew
else
	echo "No piper in sight, starting!"
	./piper --model en_US-lessac-medium.onnx --output_file output.wav &>>log.txt &
	./playnew.sh &>>log.txt &
fi

#if pgrep playnew; then
#	pkill playnew
#else
#	./playnew.sh >log.txt &
#fi
