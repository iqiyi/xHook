LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE            := xhook
LOCAL_SRC_FILES         := $(LOCAL_PATH)/../../libxhook/libs/$(TARGET_ARCH_ABI)/libxhook.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../libxhook/jni
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := biz
LOCAL_SRC_FILES         := biz.c
LOCAL_SHARED_LIBRARIES  := xhook
LOCAL_CFLAGS            := -Wall -Wextra -Werror
LOCAL_CONLYFLAGS        := -std=c11
LOCAL_LDLIBS            := -llog
include $(BUILD_SHARED_LIBRARY)
