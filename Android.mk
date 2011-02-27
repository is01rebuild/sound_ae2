LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sound
LOCAL_SRC_FILES := playwav.c data1.c data2.c data3.c data4.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

