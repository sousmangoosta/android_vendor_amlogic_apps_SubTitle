LOCAL_PATH := $(call my-dir)
DVB_PATH := $(wildcard external/dvb)
ifeq ($(DVB_PATH), )
    DVB_PATH := $(wildcard vendor/amlogic/external/dvb)
endif
ifeq ($(DVB_PATH), )
    DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif
include $(CLEAR_VARS)

LOCAL_MODULE    := libccsubjni
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := ccsub.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := external/libzvbi/src \
    vendor/amlogic/external/libzvbi/src \
    $(DVB_PATH)/include/am_mw \
    $(DVB_PATH)/include/am_adp \
    bionic/libc/include \
    vendor/amlogic/frameworks/av/LibPlayer/amcodec/include \
    vendor/amlogic/frameworks/av/LibPlayer/amadec/include \
    external/skia/include\
    vendor/amlogic/dvb/android/ndk/include\
    vendor/amlogic/dvb/android/ndk/include/linux \
    common/include/linux/amlogic

LOCAL_SHARED_LIBRARIES += libjnigraphics  libzvbi libam_mw libam_adp libskia liblog libcutils
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
