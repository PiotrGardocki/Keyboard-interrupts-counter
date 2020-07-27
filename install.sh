#!/bin/bash

# module
cd "$(dirname "$0")"
cd key-handler/src
make
DEVICE_ID=$(cat /proc/devices | grep key-handler | awk '{ print $1 }')
mknod /dev/key-handler c $DEVICE_ID 0

# application
cd ../../key-handler-api/src
gcc key-handler-api.c -o key-handler-api -std=c11
mv key-handler-api ../key-handler-api
