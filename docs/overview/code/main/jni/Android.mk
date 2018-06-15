LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE            := test
LOCAL_SRC_FILES         := $(LOCAL_PATH)/../../libtest/libs/$(TARGET_ARCH_ABI)/libtest.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../libtest/jni
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE            := main
LOCAL_SRC_FILES         := main.c
LOCAL_SHARED_LIBRARIES  := test
LOCAL_CFLAGS            := -Wall -Wextra -Werror -fPIE
LOCAL_CONLYFLAGS        := -std=c11
LOCAL_LDLIBS            += -fPIE -pie
include $(BUILD_EXECUTABLE)
