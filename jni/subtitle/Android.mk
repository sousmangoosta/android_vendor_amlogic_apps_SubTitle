LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libsubjni
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := sub_jni.c sub_api.c log_print.c sub_subtitle.c sub_vob_sub.c sub_set_sys.c vob_sub.c sub_pgs_sub.c sub_control.c avi_sub.c sub_dvb_sub.c amsysfsutils.c Amsyswrite.cpp MemoryLeakTrackUtilTmp.cpp sub_socket.cpp sub_io.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
    $(TOP)/frameworks/native/services \
    $(TOP)/frameworks/native/include

LOCAL_SHARED_LIBRARIES += \
    libutils \
    libmedia \
    libcutils \
    libbinder \
    libsystemcontrolservice \
    liblog \
    vendor.amlogic.hardware.systemcontrol@1.0

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)frameworks/services/systemcontrol \
  $(BOARD_AML_VENDOR_PATH)frameworks/services/systemcontrol/PQ/include

LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
