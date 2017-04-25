#include <am_cc.h>
#include <core/SkBitmap.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <fcntl.h>
#include <cutils/properties.h>

#include <android/bitmap.h>
extern "C" {

static JavaVM   *gJavaVM = NULL;
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG    "ccsubjni"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define INIT_W    720
#define INIT_H    576

#define AV_DEV_NO      0
#define DMX_DEV_NO     0

static jmethodID gUpdateID;
static jfieldID  gBitmapID;
int ccbitmapsize_w, ccbitmapsize_h;


typedef struct {
    pthread_mutex_t  lock;
    AM_CC_Handle_t   cc_handle;
    int              dmx_id;
    int              filter_handle;
    jobject          obj;
    SkBitmap        *bitmap;
    jobject          obj_bitmap;
    int              bmp_w;
    int              bmp_h;
    int              bmp_pitch;
    uint8_t         *buffer;
    int             sub_w;
    int             sub_h;
} DevCCData;

static DevCCData gCCData;

static DevCCData* cc_get_data(JNIEnv *env, jobject obj)
{
    return &gCCData;
}

static uint8_t *lock_bitmap(JNIEnv *env, jobject bitmap)
{
    int attached = 0;
    if (!env) {
        int ret;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return NULL;
            }
            attached = 1;
        }
    }

    uint8_t *buf;
    AndroidBitmap_lockPixels(env, bitmap, (void **) &buf);

    if (attached) {
        gJavaVM->DetachCurrentThread();
    }

    return buf;
}

static void unlock_bitmap(JNIEnv *env, jobject bitmap)
{
    int attached = 0;
    if (!env) {
        int ret;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    if (attached) {
        gJavaVM->DetachCurrentThread();
    }
}

static void notify_bitmap_changed(jobject bitmap)
{
    lock_bitmap(NULL, bitmap);
    unlock_bitmap(NULL, bitmap);
}

static uint8_t *get_bitmap(JNIEnv *env, DevCCData *cc, int *w, int *h, int *pitch)
{
    uint8_t *buf;

    buf = lock_bitmap(env, cc->obj_bitmap);
    unlock_bitmap(env, cc->obj_bitmap);
    LOGI("---bitmap buffer [%p]", buf);

    if (!buf) {
        LOGE("allocate bitmap buffer failed");
    } else {
        AndroidBitmapInfo bitmapInfo;
        AndroidBitmap_getInfo(env, cc->obj_bitmap, &bitmapInfo);
        LOGI("init bitmap info w:%d h:%d s:%d", bitmapInfo.width, bitmapInfo.height, bitmapInfo.stride);

        if (w) {
            *w = bitmapInfo.width;
        }
        if (h) {
            *h = bitmapInfo.height;
        }
        if (pitch) {
            *pitch = bitmapInfo.stride;
        }
    }

    return buf;
}

static void clear_bitmap(DevCCData *cc)
{
    uint8_t *ptr = cc->buffer;
    int y = cc->bmp_h;

    while (y--) {
        memset(ptr, 0, cc->bmp_pitch);
        ptr += cc->bmp_pitch;
    }
}

static void cc_draw_begin_cb(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para)
{
    DevCCData *cc = (DevCCData*)AM_CC_GetUserData(handle);

    pthread_mutex_lock(&cc->lock);
    if (cc && cc->buffer && cc->obj_bitmap) {
        cc->buffer = lock_bitmap(NULL, cc->obj_bitmap);
    }
}

static void sub_update(jobject obj)
{
    JNIEnv *env;
    int ret;
    int attached = 0;

    if (!obj)
        return;

    ret = gJavaVM->GetEnv((void**) &env, JNI_VERSION_1_4);

    if (ret<0) {
        ret = gJavaVM->AttachCurrentThread(&env, NULL);
        if (ret<0) {
            LOGE("Can't attach thread");
            return;
        }
        attached = 1;
    }

    env->CallVoidMethod(obj, gUpdateID, ccbitmapsize_w, ccbitmapsize_h);
    if (attached) {
        gJavaVM->DetachCurrentThread();
    }
}

static jint cc_init(JNIEnv *env, jobject obj)
{
    DevCCData *data;
    jclass bmp_clazz;
    jfieldID fid;
    jobject bmp;
    jlong hbmp;

    data = &gCCData;
    memset(data, 0, sizeof(DevCCData));

    pthread_mutex_init(&data->lock, NULL);

    data->obj = env->NewGlobalRef(obj);
    bmp = env->GetStaticObjectField(env->FindClass("com/subtitleview/CcSubView"), gBitmapID);
    data->obj_bitmap = env->NewGlobalRef(bmp);

    data->buffer = get_bitmap(env, data, &data->bmp_w, &data->bmp_h, &data->bmp_pitch);
    if (!data->buffer) {
        env->DeleteGlobalRef(data->obj);
        env->DeleteGlobalRef(data->obj_bitmap);
        pthread_mutex_destroy(&data->lock);
        return -1;
    }
    LOGI("init w:%d h:%d p:%d", data->bmp_w, data->bmp_h, data->bmp_pitch);
    /*just init, no effect*/
    data->sub_w = INIT_W;
    data->sub_h = INIT_H;

    return 0;
}

static jint cc_destroy(JNIEnv *env, jobject obj)
{
    DevCCData *data = cc_get_data(env, obj);

    if (data) {

        if (data->obj) {
            env->DeleteGlobalRef(data->obj);
            data->obj = NULL;
        }
        if (data->obj_bitmap) {
            env->DeleteGlobalRef(data->obj_bitmap);
            data->obj_bitmap = NULL;
        }
        pthread_mutex_destroy(&data->lock);
    }

    return 0;
}

static jint cc_lock(JNIEnv *env, jobject obj)
{
    DevCCData *data = cc_get_data(env, obj);

    pthread_mutex_lock(&data->lock);

    return 0;
}

static jint cc_unlock(JNIEnv *env, jobject obj)
{
    DevCCData *data = cc_get_data(env, obj);

    pthread_mutex_unlock(&data->lock);

    return 0;
}

static void cc_draw_end_cb(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para)
{
    DevCCData *cc = (DevCCData*)AM_CC_GetUserData(handle);

    unlock_bitmap(NULL, cc->obj_bitmap);

    cc->sub_w = draw_para->caption_width;
    cc->sub_h = draw_para->caption_height;
    ccbitmapsize_w = draw_para->caption_width;
    ccbitmapsize_h = draw_para->caption_height;

    pthread_mutex_unlock(&cc->lock);
    LOGE("cc_draw_end_cb");
    sub_update(cc->obj);
}

static jint dev_start_atsc_cc(JNIEnv *env, jobject obj, jint caption, jint fg_color,
    jint fg_opacity, jint bg_color, jint bg_opacity, jint font_style, jint font_size)
{
    DevCCData *data = cc_get_data(env, obj);
    AM_CC_CreatePara_t cc_para;
    AM_CC_StartPara_t spara;
    int ret;

    LOGI("start cc: caption %d, fgc %d, bgc %d, fgo %d, bgo %d, fsize %d, fstyle %d",
        caption, fg_color, bg_color, fg_opacity, bg_opacity, font_size, font_style);

    memset(&cc_para, 0, sizeof(cc_para));
    memset(&spara, 0, sizeof(spara));

    cc_para.bmp_buffer = data->buffer;
    cc_para.pitch = data->bmp_pitch;
    cc_para.draw_begin = cc_draw_begin_cb;
    cc_para.draw_end = cc_draw_end_cb;
    cc_para.user_data = (void*)data;

    spara.caption                  = (AM_CC_CaptionMode_t)caption;
    spara.user_options.bg_color    = (AM_CC_Color_t)bg_color;
    spara.user_options.fg_color    = (AM_CC_Color_t)fg_color;
    spara.user_options.bg_opacity  = (AM_CC_Opacity_t)bg_opacity;
    spara.user_options.fg_opacity  = (AM_CC_Opacity_t)fg_opacity;
    spara.user_options.font_size   = (AM_CC_FontSize_t)font_size;
    spara.user_options.font_style  = (AM_CC_FontStyle_t)font_style;

    ret = AM_CC_Create(&cc_para, &data->cc_handle);
    if (ret != AM_SUCCESS)
        goto error;

    ret = AM_CC_Start(data->cc_handle, &spara);
    if (ret != AM_SUCCESS)
        goto error;

    LOGI("start cc successfully!");
    return 0;
error:
    if (data->cc_handle != NULL) {
        AM_CC_Destroy(data->cc_handle);
    }
    LOGI("start cc failed!");
    return -1;
}

static jint dev_stop_atsc_cc(JNIEnv *env, jobject obj)
{
    DevCCData *data = cc_get_data(env, obj);

    LOGI("stop cc");
    AM_CC_Destroy(data->cc_handle);

    pthread_mutex_lock(&data->lock);
    data->buffer = lock_bitmap(env, data->obj_bitmap);
    clear_bitmap(data);
    unlock_bitmap(env, data->obj_bitmap);
    pthread_mutex_unlock(&data->lock);

    sub_update(data->obj);

    data->cc_handle= NULL;

    return 0;
}

static jint cc_set_active(JNIEnv *env, jobject obj, jboolean active)
{
    DevCCData *data = cc_get_data(env, obj);

    if (active) {
        if (data->obj) {
            env->DeleteGlobalRef(data->obj);
            data->obj = NULL;
        }

        data->obj = env->NewGlobalRef(obj);
    } else {
        if (env->IsSameObject(data->obj, obj)) {
            env->DeleteGlobalRef(data->obj);
            data->obj = NULL;
        }
    }

    return 0;
}

static JNINativeMethod gMethods[] = {
        /* name, signature, funcPtr */
        {"native_cc_init", "()I", (void*)cc_init},
        {"native_cc_destroy", "()I", (void*)cc_destroy},
        {"native_cc_lock", "()I", (void*)cc_lock},
        {"native_cc_unlock", "()I", (void*)cc_unlock},
        {"native_start_atsc_cc", "(IIIIIII)I", (void*)dev_start_atsc_cc},
        {"native_stop_atsc_cc", "()I", (void*)dev_stop_atsc_cc},
        {"native_cc_set_active", "(Z)I", (void*)cc_set_active}
};


JNIEXPORT jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGE("in JNI_OnLoad\n");
    JNIEnv* env = NULL;
    jclass clazz;
    int rc;

    gJavaVM = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed");
        return -1;
    }
    LOGE("GetEnv ok");

    clazz = env->FindClass("com/subtitleview/CcSubView");
    if (clazz == NULL) {
        LOGE("FindClass com/subtitleview/CcSubView failed");
        return -1;
    }
    LOGI("find  CcSubView class ok");
    if ((rc = (env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/sizeof(gMethods[0])))) < 0) {
        LOGE("RegisterNatives failed");
        return -1;
    }
    LOGI("RegisterNatives subfunc ok");
    gUpdateID = env->GetMethodID(clazz, "update", "(II)V");

    gBitmapID = env->GetStaticFieldID(clazz, "bitmap", "Landroid/graphics/Bitmap;");

    LOGI("load jnitestplay ok");
    return JNI_VERSION_1_4;
}

JNIEXPORT void
JNI_OnUnload(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return;
    }
}

}
