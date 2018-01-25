#!/bin/bash

cp -fr ./libxhook/libs/armeabi   ./xhookwrapper/xhook/libs/armeabi
cp -fr ./libxhook/libs/arm64-v8a ./xhookwrapper/xhook/libs/arm64-v8a

cp -fr ./libbiz/libs/armeabi     ./xhookwrapper/biz/libs/armeabi
cp -fr ./libbiz/libs/arm64-v8a   ./xhookwrapper/biz/libs/arm64-v8a

cp -fr ./libtest/libs/armeabi    ./xhookwrapper/app/libs/armeabi
cp -fr ./libtest/libs/arm64-v8a  ./xhookwrapper/app/libs/arm64-v8a
