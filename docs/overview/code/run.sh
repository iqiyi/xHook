#!/bin/bash -e

adb push ./main/libs/armeabi-v7a/libtest.so ./main/libs/armeabi-v7a/main /data/local/tmp
adb shell "chmod 777 /data/local/tmp/main"
adb shell "export LD_LIBRARY_PATH=/data/local/tmp; /data/local/tmp/main"
