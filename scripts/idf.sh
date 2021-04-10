#!/usr/bin/env sh

idf.py -p /dev/ttyUSB0 -b 921600 $* || exit 1
