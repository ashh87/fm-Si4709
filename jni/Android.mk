LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := fm
LOCAL_SRC_FILES := fm.c
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS := -llog
LOCAL_INCLUDES += $(LOCAL_PATH)

LOCAL_PRELINK_MODULE := false

#include $(BUILD_SHARED_LIBRARY) 
include $(BUILD_EXECUTABLE)