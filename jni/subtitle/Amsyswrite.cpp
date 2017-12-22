#define LOG_TAG "amSystemControl"

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <Amsyswrite.h>
#include <unistd.h>

#include <MemoryLeakTrackUtilTmp.h>
#include <fcntl.h>

#include <../../../../frameworks/services/systemcontrol/SystemControlClient.h>

using namespace android;

class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier()
        {
        } void binderDied(const wp < IBinder > &who)
        {
            ALOGW("system_write died!");
        }
};

static sp < DeathNotifier > amDeathNotifier;
static Mutex amLock;
static Mutex amgLock;
static SystemControlClient *mScc = new SystemControlClient();

int amSystemControlGetProperty(const char* key, char* value)
{
    if (mScc != NULL)
    {
        const std::string stdKey(key);
        std::string stdValue(value);
        if (mScc->getProperty(stdKey, stdValue))
        {
            return 0;
        }
    }
    return -1;
}

int amSystemControlGetPropertyStr(const char* key, char* def, char* value)
{
    if (mScc != NULL) {
        const std::string stdKey(key);
        std::string stdDef(def);
        std::string stdValue(value);
        mScc->getPropertyString(stdKey, stdValue, stdDef);
    }
    return -1;
}

int amSystemControlGetPropertyInt(const char* key, int def)
{
    if (mScc != NULL)
    {
        const std::string stdKey(key);
        return mScc->getPropertyInt(stdKey, def);
    }
    return def;
}

long amSystemControlGetPropertyLong(const char* key, long def)
{
    if (mScc != NULL)
    {
        const std::string stdKey(key);
        return mScc->getPropertyLong(stdKey, (int64_t)def);
    }
    return def;
}

int amSystemControlGetPropertyBool(const char* key, int def)
{
    if (mScc != NULL)
    {
        const std::string stdKey(key);
        if (mScc->getPropertyBoolean(stdKey, def))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return def;
}

void amSystemControlSetProperty(const char* key, const char* value)
{
    if (mScc != NULL)
    {
        const std::string stdKey(key);
        const std::string stdValue(value);
        mScc->setProperty(stdKey, stdValue);
    }
}

int amSystemControlReadSysfs(const char* path, char* value)
{
    if (mScc != NULL)
    {
        const std::string stdPath(path);
        std::string stdValue(value);
        if (mScc->readSysfs(stdPath, stdValue))
        {
            ALOGE("amSystemControlReadSysfs stdValue:%s", stdValue.c_str());
            return 0;
        }
    }
    return -1;
}

int amSystemControlReadNumSysfs(const char* path, char* value, int32_t size)
{
    //ALOGD("amSystemControlReadNumSysfs:%s",path);
    //const sp < ISystemControlService > &scs = getSystemControlService();
    if (mScc != NULL /*&& value != NULL && access(path, 0) != -1*/)
    {
        //String16 v;
        /*if (mScc->readSysfs(path, value))
        {
            if (value.size() != 0)
            {
                if (size <= value.size() + 1)
                {
                    value[size] = '\0';
                }
                return 0;
            }
        }*/
    }
    //ALOGD("[false]amSystemControlReadNumSysfs%s,",path);
    return -1;
}

int amSystemControlWriteSysfs(const char* path, char* value)
{
    if (mScc != NULL)
    {
        const std::string stdPath(path);
        std::string stdValue(value);
        if (mScc->writeSysfs(stdPath, stdValue))
        {
            return 0;
        }
    }
    return -1;
}

void amDumpMemoryAddresses(int fd)
{
    ALOGE("[amDumpMemoryAddresses]fd:%d\n", fd);
    dumpMemoryAddresses(fd);
    close(fd);
}
