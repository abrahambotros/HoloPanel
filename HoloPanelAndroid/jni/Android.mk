LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq (1, $(NDK_DEBUG))
LOCAL_CFLAGS += -DWAIT_FOR_DEBUGGER
endif

# Tegra optimized OpenCV.mk
include $(CLEAR_VARS)
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
include $(OPENCV_PATH)/sdk/native/jni/OpenCV-tegra3.mk
#include $(OPENCV_PATH)/sdk/native/jni/OpenCV.mk

# HoloPanel
LOCAL_LDLIBS += -llog
LOCAL_MODULE    := HoloPanel
LOCAL_SRC_FILES := MainActivity.cpp HoloPanel/Common.cpp HoloPanel/HoloPanelApp.cpp

include $(BUILD_SHARED_LIBRARY)