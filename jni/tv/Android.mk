LOCAL_PATH := $(call my-dir)

DVB_PATH := $(wildcard external/dvb)
ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/common/external/dvb)
endif
ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/common/dvb)
endif

#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libtvsubtitle_tv
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := DTVSubtitle.cpp
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -DSUPPORT_ADTV
LOCAL_CFLAGS += -DUSE_VENDOR_ICU
LOCAL_C_INCLUDES := external/libzvbi/src \
  $(DVB_PATH)/include/am_mw \
  $(DVB_PATH)/include/am_adp \
  bionic/libc/include \
  external/skia/include\
  $(DVB_PATH)/android/ndk/include \
  vendor/amlogic/common/external/libzvbi/src \
  $(JNI_H_INCLUDE) \


LOCAL_SHARED_LIBRARIES += libzvbi libam_mw libam_adp  liblog libcutils

LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)


#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libjnifont_tv
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := Fonts.cpp
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
    $(JNI_H_INCLUDE) \
    libnativehelper/include_jni \
    system/core/libutils/include \
    system/core/liblog/include \
    libnativehelper/include/nativehelper
LOCAL_SHARED_LIBRARIES += libvendorfont  liblog  libcutils

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_PRELINK_MODULE := false

#include $(BUILD_SHARED_LIBRARY)

#######################################################################
