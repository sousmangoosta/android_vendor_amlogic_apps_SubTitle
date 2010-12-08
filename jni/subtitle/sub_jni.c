////////////////////////////////////////////////////////////////////////////////
// JNI Interface
////////////////////////////////////////////////////////////////////////////////

#include <jni.h>
#include <android/log.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


#include "sub_set_sys.h"

#define  LOG_TAG    "sub_jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#include "sub_api.h"
#include <string.h>

JNIEXPORT jobject JNICALL parseSubtitleFile
  (JNIEnv *env, jclass cl, jstring filename, jstring encode)
{
      jclass cls = (*env)->FindClass(env, "com/subtitleparser/SubtitleFile");
      if(!cls){
          LOGE("parseSubtitleFile: failed to get SubtitleFile class reference");
          return NULL;
      }

      jmethodID constr = (*env)->GetMethodID(env, cls, "<init>", "()V");
      if(!constr){
          LOGE("parseSubtitleFile: failed to get  constructor method's ID");
          return NULL;
      }

      jobject obj =  (*env)->NewObject(env, cls, constr);
      if(!obj){
          LOGE("parseSubtitleFile: failed to create an object");
          return NULL;
      }

	  jmethodID mid = (*env)->GetMethodID(env, cls, "appendSubtitle", "(III[BLjava/lang/String;)V");
      if(!mid){
          LOGE("parseSubtitleFile: failed to get method append's ID");
          return NULL;
      }

      const char *nm = (*env)->GetStringUTFChars(env,filename, NULL);
      const char* charset= (*env)->GetStringUTFChars(env,encode, NULL);
      subdata_t * subdata = NULL;
      subdata = internal_sub_open(nm,0,charset);
      if(subdata == NULL){
          LOGE("internal_sub_open failed! :%s",nm);
          goto err2;
      }

      jint i=0, j=0;
      list_t *entry;
      jstring jtext;
      
      char * textBuf = NULL;

      list_for_each(entry, &subdata->list)
      {
          i++;
          subtitle_t *subt = list_entry(entry, subtitle_t, list);
	
		  textBuf = (char *)malloc(subt->text.lines *512);
		  if(textBuf == NULL){
			LOGE("malloc text buffer failed!");
			goto err;
		  }

		memset( textBuf, 0,subt->text.lines *512);

		for (j=0; j< subt->text.lines; j++) {
			strcat(textBuf, subt->text.text[j]);
			strcat(textBuf, "\n");
		}
		
	  jbyteArray array= (*env)->NewByteArray(env,strlen(textBuf));
	  
	  (*env)->SetByteArrayRegion(env,array,0,strlen(textBuf), textBuf);	  
		//jtext = (*env)->NewStringUTF(env, textBuf); //may cause err.
              

		(*env)->CallVoidMethod(env, obj, mid, i, subt->start/90, subt->end/90, array,encode);
		(*env)->DeleteLocalRef (env,array );
		free(textBuf);
	}
	internal_sub_close(subdata);
	(*env)->ReleaseStringUTFChars(env,filename, nm);
	return obj;

err:


      internal_sub_close(subdata);

err2:

      (*env)->ReleaseStringUTFChars(env,filename, nm);


      return NULL;
  }
JNIEXPORT jint JNICALL getInSubtitleTotal
  (JNIEnv *env, jclass cl)  
{
	int subtitle_num = get_subtitle_num();
    LOGE("jni getInSubtitleTotal!");
    if(subtitle_num > 0)
    	return subtitle_num;
	return 0;
}

JNIEXPORT jint JNICALL setInSubtitleNumber
  (JNIEnv *env, jclass cl, jint index )  
{
    LOGE("jni setInSubtitleNumber!");
	return 0;
}

JNIEXPORT jint JNICALL getCurrentInSubtitleIndex
  (JNIEnv *env, jclass cl )  
{
	int subtitle_curr = get_subtitle_curr();
    LOGE("jni getCurrentInSubtitleIndex!");
	if(subtitle_curr >= 0)
    	return subtitle_curr;
	return -1;
}



JNIEXPORT jobject JNICALL getrawdata
	  (JNIEnv *env, jclass cl, jint msec )  
{
    LOGE("jni getdata!,eed return a java object:RawData");
	jclass cls = (*env)->FindClass(env, "com/subtitleparser/subtypes/RawData");
	if(!cls){
		LOGE("com/subtitleparser/subtypes/RawData: failed to get RawData class reference");
		return NULL;
	}
	
	jmethodID constr = (*env)->GetMethodID(env, cls, "<init>", "([IIIIILjava/lang/String;)V");
	if(!constr){
		LOGE("com/subtitleparser/subtypes/RawData: failed to get  constructor method's ID");
	  return NULL;
	}
	
	
	LOGE("start get packet\n\n");
	int sub_pkt = get_inter_spu_packet(msec*90);
	if(sub_pkt < 0){
		LOGE("sub pkt fail\n\n");
		return NULL;
	}
	
	int sub_size = get_inter_spu_size();
	if(sub_size <= 0){
		LOGE("sub_size invalid \n\n");
		return NULL;
	}	
	LOGE("sub_size is %d\n\n",sub_size);
	int *inter_sub_data = NULL;
	inter_sub_data = malloc(sub_size*4);
	if(inter_sub_data == NULL){
		LOGE("malloc sub_size fail \n\n");
		return NULL;
	}
	memset(inter_sub_data, 0x0, sub_size*4);
	LOGE("start get new array\n\n");
	jintArray array= (*env)->NewIntArray(env,sub_size);
	if(!array){
		LOGE("new int array fail \n\n");
		return NULL;
	}
 
	parser_inter_spu(inter_sub_data);
	LOGE("end parser_inter_spu\n\n");
	(*env)->SetIntArrayRegion(env,array,0,sub_size, inter_sub_data);	 
	LOGE("start get new object\n\n");
	free(inter_sub_data);
	jobject obj =  (*env)->NewObject(env, cls, constr,array,1,get_inter_spu_width(),
		get_inter_spu_height(),get_inter_spu_delay(),0);
	add_read_position();
	if(!obj){
	  LOGE("parseSubtitleFile: failed to create an object");
	  return NULL;
	}
	//(*env)->CallVoidMethod(env, obj, constr, array, 1, get_inter_spu_width(),
		//get_inter_spu_height(),0);
	
    LOGE("jni getdata!,eed return a java object:RawData");
	return obj;

}

//JNIEXPORT jobject JNICALL getdata
//	  (JNIEnv *env, jclass cl, jint msec )  
//{
//    LOGE("jni getdata!,eed return a java object:SubData");
//	return NULL;
//
//}
void inter_subtitle_parser()
{
	if(get_subtitle_enable()&&get_subtitle_num())
		get_inter_spu();
}


int subtitle_thread_create()
{
#if 0
	pthread_t thread;
    int rc;
    LOGI("[subtitle_thread:%d]starting controler thread!!\n", __LINE__);
    rc = pthread_create(&thread, NULL, inter_subtitle_parser, NULL);
    if (rc) {
        LOGE("[subtitle_thread:%d]ERROR; start failed rc=%d\n", __LINE__,rc);
    }
	return rc;

	struct sigaction act; 
    union sigval tsval; 

    act.sa_handler = inter_subtitle_parser; 
    act.sa_flags = 0; 
    sigemptyset(&act.sa_mask); 
    sigaction(50, &act, NULL); 

    while(1)
    { 
        usleep(300); 
        sigqueue(getpid(), 50, tsval); 
    } 
    return 0; 
#else

    struct sigaction tact; 
    
    tact.sa_handler = inter_subtitle_parser; 
    tact.sa_flags = 0; 
    sigemptyset(&tact.sa_mask); 
    sigaction(SIGALRM, &tact, NULL); 

	struct itimerval value; 
    
    value.it_value.tv_sec = 1; 
    value.it_value.tv_usec = 0;//500000; 
    value.it_interval = value.it_value; 
    setitimer(ITIMER_REAL, &value, NULL);
    //while(1);
	return 0;
#endif

}

static JNINativeMethod gMethods[] = {
    /* name, signature, funcPtr */
    { "parseSubtitleFileByJni", "(Ljava/lang/String;Ljava/lang/String;)Lcom/subtitleparser/SubtitleFile;",
            (void*) parseSubtitleFile},
    };

static JNINativeMethod insubMethods[] = {
    /* name, signature, funcPtr */
    	{ "getInSubtitleTotalByJni", "()I",                                 (void*)getInSubtitleTotal},
		{ "setInSubtitleNumberByJni", "(I)I",                               (void*)setInSubtitleNumber},
		{ "getCurrentInSubtitleIndexByJni", "()I",                          (void*)getCurrentInSubtitleIndex },
    };
    
static JNINativeMethod insubdataMethods[] = {
    /* name, signature, funcPtr */
    	{ "getrawdata", "(I)Lcom/subtitleparser/subtypes/RawData;", (void*)getrawdata},
//    	{ "getdataByJni", "(I)Lcom/subtitleparser/subtypes/InSubApi;", (void*)getdata},

    };    

static int registerNativeMethods(JNIEnv* env, const char* className,
                                 const JNINativeMethod* methods, int numMethods)
{
    int rc;
    jclass clazz;
    clazz = (*env)->FindClass(env, className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (rc = ((*env)->RegisterNatives(env, clazz, methods, numMethods)) < 0) {
        LOGE("RegisterNatives failed for '%s' %d\n", className, rc);
        return -1;
    }

    return 0;
}

JNIEXPORT jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jclass * localClass;
    LOGE("================= JNI_OnLoad ================\n");

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed!");
        return -1;
    }

    if (registerNativeMethods(env, "com/subtitleparser/Subtitle",gMethods, NELEM(gMethods)) < 0){
        LOGE("registerNativeMethods failed!");
        return -1;
    }
	if (registerNativeMethods(env, "com/subtitleparser/SubtitleUtils",insubMethods, NELEM(insubMethods)) < 0){
		LOGE("registerNativeMethods failed!");
		return -1;
    }
	if (registerNativeMethods(env, "com/subtitleparser/subtypes/InSubApi",insubdataMethods, NELEM(insubdataMethods)) < 0){
		LOGE("registerNativeMethods failed!");
		return -1;
    }    
//	subtitle_thread_create();
    return JNI_VERSION_1_4;
}