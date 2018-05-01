LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE     := test
LOCAL_SRC_FILES  := test.c
LOCAL_CFLAGS     := -Wall -Wextra -Werror #-O0
LOCAL_CONLYFLAGS := -std=c11
include $(BUILD_SHARED_LIBRARY)
