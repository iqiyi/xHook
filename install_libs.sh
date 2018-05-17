#!/bin/bash

mkdir -p ./xhookwrapper/xhook/libs/armeabi
mkdir -p ./xhookwrapper/xhook/libs/armeabi-v7a
mkdir -p ./xhookwrapper/xhook/libs/arm64-v8a
mkdir -p ./xhookwrapper/xhook/libs/x86
mkdir -p ./xhookwrapper/xhook/libs/x86_64

cp -f ./libxhook/libs/armeabi/libxhook.so     ./xhookwrapper/xhook/libs/armeabi/
cp -f ./libxhook/libs/armeabi-v7a/libxhook.so ./xhookwrapper/xhook/libs/armeabi-v7a/
cp -f ./libxhook/libs/arm64-v8a/libxhook.so   ./xhookwrapper/xhook/libs/arm64-v8a/
cp -f ./libxhook/libs/x86/libxhook.so         ./xhookwrapper/xhook/libs/x86/
cp -f ./libxhook/libs/x86_64/libxhook.so      ./xhookwrapper/xhook/libs/x86_64/

mkdir -p ./xhookwrapper/biz/libs/armeabi
mkdir -p ./xhookwrapper/biz/libs/armeabi-v7a
mkdir -p ./xhookwrapper/biz/libs/arm64-v8a
mkdir -p ./xhookwrapper/biz/libs/x86
mkdir -p ./xhookwrapper/biz/libs/x86_64

cp -f ./libbiz/libs/armeabi/libbiz.so         ./xhookwrapper/biz/libs/armeabi/
cp -f ./libbiz/libs/armeabi-v7a/libbiz.so     ./xhookwrapper/biz/libs/armeabi-v7a/
cp -f ./libbiz/libs/arm64-v8a/libbiz.so       ./xhookwrapper/biz/libs/arm64-v8a/
cp -f ./libbiz/libs/x86/libbiz.so             ./xhookwrapper/biz/libs/x86/
cp -f ./libbiz/libs/x86_64/libbiz.so          ./xhookwrapper/biz/libs/x86_64/

mkdir -p ./xhookwrapper/app/libs/armeabi
mkdir -p ./xhookwrapper/app/libs/armeabi-v7a
mkdir -p ./xhookwrapper/app/libs/arm64-v8a
mkdir -p ./xhookwrapper/app/libs/x86
mkdir -p ./xhookwrapper/app/libs/x86_64

cp -f ./libtest/libs/armeabi/libtest.so       ./xhookwrapper/app/libs/armeabi/
cp -f ./libtest/libs/armeabi-v7a/libtest.so   ./xhookwrapper/app/libs/armeabi-v7a/
cp -f ./libtest/libs/arm64-v8a/libtest.so     ./xhookwrapper/app/libs/arm64-v8a/
cp -f ./libtest/libs/x86/libtest.so           ./xhookwrapper/app/libs/x86/
cp -f ./libtest/libs/x86_64/libtest.so        ./xhookwrapper/app/libs/x86_64/
