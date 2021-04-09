#!/usr/bin/env sh

idf.py flash -p /dev/ttyUSB0 -b 921600 || exit 1
