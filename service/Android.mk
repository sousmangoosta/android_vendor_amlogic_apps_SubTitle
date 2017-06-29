LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  ISubTitleService.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libbinder \
  liblog

LOCAL_MODULE:= libsubtitleservice

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)