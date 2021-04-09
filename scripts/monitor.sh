#!/usr/bin/env sh

idf.py monitor -p /dev/ttyUSB0 || exit 1
