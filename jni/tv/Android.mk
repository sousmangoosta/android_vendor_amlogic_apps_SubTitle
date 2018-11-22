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
LOCAL_C_INCLUDES := external/libzvbi/src \
  $(DVB_PATH)/include/am_mw \
  $(DVB_PATH)/include/am_adp \
  bionic/libc/include \
  external/skia/include\
  $(DVB_PATH)/android/ndk/include \
  vendor/amlogic/common/external/libzvbi/src

LOCAL_SHARED_LIBRARIES += libjnigraphics libzvbi libam_mw libam_adp  liblog libcutils

LOCAL_PRELINK_MODULE := false

#include $(BUILD_SHARED_LIBRARY)

#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libjnivendorfont
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional

ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_MODULE_PATH_64 := $(TARGET_OUT)/lib64
LOCAL_SRC_FILES_64 := arm64/libjnivendorfont.so
LOCAL_MODULE_PATH_32 := $(TARGET_OUT)/lib
LOCAL_SRC_FILES_32 := arm/libjnivendorfont.so
else
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := arm/libjnivendorfont.so
endif
#include $(BUILD_PREBUILT)

#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libjnifont_tv
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := Fonts.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)
LOCAL_SHARED_LIBRARIES += libjnivendorfont liblog libnativehelper libandroid_runtime libcutils

LOCAL_PRELINK_MODULE := false

#include $(BUILD_SHARED_LIBRARY)

#######################################################################
