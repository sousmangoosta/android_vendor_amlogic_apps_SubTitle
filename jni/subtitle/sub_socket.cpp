/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "sub_socket"

#include <fcntl.h>
#include <utils/Log.h>
//#include <binder/IPCThreadState.h>
//#include <binder/IServiceManager.h>
//#include <binder/PermissionCache.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/stat.h>
#include<pthread.h>
#include <inttypes.h>

#include "sub_socket.h"

//using namespace android;

/**
 * Safe copy for loop buffer, check edge before copy
 * @param
 * sPtr : loop buffer start pointer
 * mWPtr : current write pointer
 * mRPtr : current read pointer
 * src : source data for copy
 * size : source data size
 */
void safeCopy(char* sPtr, char* src, int size) {
    char* ptrStart = sPtr;
    char* ptrEnd = sPtr + LOOP_BUFFER_SIZE;
    int leftReg = 0;//from nearest wptr to ptrEnd

    //skip case for data recover, which means write ptr is 256*1024 bigger than read ptr
    leftReg = ptrEnd - mWPtr;
    ALOGV("[safeCopy]sPtr:0x%x, mWPtr:0x%x, mRPtr:0x%x, size:%d, leftReg:%d\n", sPtr, mWPtr, mRPtr, size, leftReg);
    if (mWPtr != 0) {
        if (leftReg >= size) {
            memcpy(mWPtr, src, size);
            mWPtr += size;
        }
        else {
            memcpy(mWPtr, src, leftReg);
            mWPtr = sPtr;
            memcpy(mWPtr, (src + leftReg), (size - leftReg));
            mWPtr += (size - leftReg);
        }
    }
}

void safeRead(char* sPtr, char* des, int size) {
    char* ptrStart = sPtr;
    char* ptrEnd = sPtr + LOOP_BUFFER_SIZE;
    int leftReg = 0;//from nearest rptr to ptrEnd

    leftReg = ptrEnd - mRPtr;
    ALOGV("[safeRead]sPtr:0x%x,mWPtr:0x%x, mRPtr:0x%x, size:%d, leftReg:%d\n", sPtr, mWPtr, mRPtr, size, leftReg);
    if (mRPtr != 0) {
        if (leftReg >= size) {
            memcpy(des, mRPtr, size);
            mRPtr += size;
        }
        else {
            memcpy(des, mRPtr, leftReg);
            mRPtr = sPtr;
            memcpy((des + leftReg), mRPtr, (size - leftReg));
            mRPtr += (size - leftReg);
        }
    }
}

int getDataSize(char* sPtr) {
    char* ptrStart = sPtr;
    char* ptrEnd = sPtr + LOOP_BUFFER_SIZE;
    int size = 0;

    if (mWPtr >= mRPtr) {
        size = mWPtr - mRPtr;
    }
    else {
        size = (ptrEnd - mRPtr) + (mWPtr - ptrStart);
    }
    //ALOGV("[getDataSize]mWPtr:0x%x, mRPtr:0x%x, size:%d\n", mWPtr, mRPtr, size);

    return size;
}

void* startServerThread(void* arg) {
    ALOGE("[startServerThread]isStartServerTread:%d\n",isStartServerTread);
    if (!isStartServerTread) {
        isStartServerTread = true;
        struct sockaddr_in addr;
        memset((void *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(LISTEN_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        mSockFd = socket(AF_INET, SOCK_STREAM, 0);

        mStop = false;
        mLoopBuf = NULL;
        /*mSockFd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "/dev/socket/sub_socket");
        int ret = unlink(addr.sun_path);
        if (ret != 0 && errno != ENOENT) {
            ALOGE("Failed to unlink old socket '%s': %s\n", addr.sun_path, strerror(errno));
            return NULL;
         }*/
         int on = 1;
         if ((setsockopt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
             ALOGE("setsockopt failed.\n");
             exit(1);
         }
         if (bind(mSockFd,(struct sockaddr *)&addr,sizeof(addr)) == -1) {
             ALOGE("bind fail. error=%d, err:%s\n", errno, strerror(errno));
             return NULL;
         }
         /*if (fchmodat(AT_FDCWD, addr.sun_path, (mode_t)0666, AT_SYMLINK_NOFOLLOW)) {
             ALOGE("fchmodat %s fail.error=%d, err:%s\n", addr.sun_path, errno, strerror(errno));
             return NULL;;
         }*/
         if (listen(mSockFd, QUEUE_SIZE) == -1) {
             ALOGE("listen fail.error=%d, err:%s\n", errno, strerror(errno));
             return NULL;;
         }
         ALOGV("[startServerThread]listen success.\n");
         while (!mStop) {
             struct sockaddr_in client_addr;
             socklen_t length = sizeof(client_addr);
             int connFd = accept(mSockFd, (struct sockaddr*)&client_addr, &length);
             if (client_num++ > 9) {
                 client_num= 9;
                 close(mSockFd);
                 ALOGV("client number is limited");
                 continue;
             }
             if (connFd < 0) {
                 ALOGE("client connect fail.error=%d, err:%s\n", errno, strerror(errno));
                 exit(1);
             }
             ALOGV("new client accepted.\n");
             child_connect(connFd);
             /*pid_t childid;
             if (childid = fork() == 0) {
                 ALOGV("child process: %d created.\n", getpid());
                 close(mSockFd);
                 child_connect(connFd);
                 exit(0);
              }*/
        }
        ALOGV("closed.\n");
        close(mSockFd);
    }
    return NULL;
}

void setInSubTypeLanBySkt(char *subInfoS) {
    //ALOGV("[setInSubTypeLanBySkt]subInfoStr:%s\n",subInfoS);
    char *ptrColon = strchr(subInfoS, '/');
    if (ptrColon != NULL) {
        strncpy(subTypeStr, subInfoS, ptrColon - subInfoS);
        strcpy(subLanStr, ptrColon + 1);
        //ALOGV("[setInSubTypeLanBySkt]subTypeStr:%s,subLanStr:%s\n",subTypeStr,subLanStr);
    }
}

void child_connect(int sockfd) {
    char recvBuf[BUFFER_SIZE] = {0};
    char sendBuf[32] = {0};
    int retLen = 0;
    char retLenStr[32] = {0};
    mClientStop = false;

    if (mLoopBuf == NULL) {
        mLoopBuf = (char *)malloc(LOOP_BUFFER_SIZE);
        memset(mLoopBuf, 0, LOOP_BUFFER_SIZE);
        mRPtr = mLoopBuf;
        mWPtr = mLoopBuf;
    }

    pid_t pid = getpid();
    client_list[client_num] = sockfd;
    while (!mClientStop) {
        //ALOGV("[child_connect]recv begin...\n");
        retLen = recv(sockfd, recvBuf, sizeof(recvBuf), 0);
        if (retLen < 0) {
            ALOGE("child recv fail, retLen: %d\n", retLen);
            break;
        }
        if (!strncmp(recvBuf,"exit\n", 5)) {
            ALOGV("child process: %d exited.\n", pid);
            break;
        }
        if (!strncmp(recvBuf,"reset\n", 6)) {
            ALOGV("child process: %d reset.\n", pid);
            resetSocketBuffer();
            continue;
        }

        if (recvBuf[0] == 0x53
            && recvBuf[1] == 0x54
            && recvBuf[2] == 0x4F
            && recvBuf[3] == 0x54
            && mTotal < 0) {//STOT
            mTotal = (recvBuf[4] << 24)
                    | (recvBuf[5] << 16)
                    | (recvBuf[6] << 8)
                    | recvBuf[7];
            ALOGV("child recv, mTotal:%d\n", mTotal);
            strcpy(subInfoStr, recvBuf+8);
            //ALOGE("child recv, subTypeStr:%s\n", subInfoStr);
            setInSubTypeLanBySkt(subInfoStr);
        }
        else if (recvBuf[0] == 0x53
            && recvBuf[1] == 0x50
            && recvBuf[2] == 0x54
            && recvBuf[3] == 0x53
            && mStartPts < 0) {//SPTS
            mStartPts = (recvBuf[4] << 24)
                    | (recvBuf[5] << 16)
                    | (recvBuf[6] << 8)
                    | recvBuf[7];
            //ALOGV("child recv, mStartPts:%" PRId64 "\n", mStartPts);
        }
        else if (recvBuf[0] == 0x53
            && recvBuf[1] == 0x54
            && recvBuf[2] == 0x59
            && recvBuf[3] == 0x50
            && mType < 0) {//STYP
            mType = (recvBuf[4] << 24)
                    | (recvBuf[5] << 16)
                    | (recvBuf[6] << 8)
                    | recvBuf[7];
            //ALOGV("child recv, mType:%d\n", mType);
        }
        else if (recvBuf[0] == 0x53
            && recvBuf[1] == 0x52
            && recvBuf[2] == 0x44
            && recvBuf[3] == 0x54) {//SRDT //subtitle render time
            mTimeUs = (recvBuf[4] << 24)
                    | (recvBuf[5] << 16)
                    | (recvBuf[6] << 8)
                    | recvBuf[7];
            //ALOGV("child recv, mTimeUs:%d\n", mTimeUs);
        }
        else if (recvBuf[0] != 'S' && recvBuf[1] != 'T' && recvBuf[2] != 'Y' && recvBuf[3] != 'P') {
            safeCopy(mLoopBuf, recvBuf, retLen);
        }

        // reveived buffer length send to client
        /*memset(sendBuf, 0, 32);
        memset(retLenStr, 0, 32);
        strcat(sendBuf, "server:received buf len:");
        sprintf(retLenStr, "%d", retLen);
        strcat(sendBuf, retLenStr);
        send(sockfd, sendBuf, 32, 0);*/
    }
    close(sockfd);
    client_num--;
}

/*void childConnectThread(int sockfd) {
    pthread_t cct;
    pthread_create(&cct, NULL, child_connect, &sockfd);
}*/

void startServer() {
    mTotal = -1;
    mStartPts = -1;
    mType = -1;

    pthread_t sst;
    pthread_create(&sst, NULL, startServerThread, NULL);
    //pthread_join(sst, NULL);
}

void stopServer() {
    mTotal = -1;
    mStartPts = -1;
    mType = -1;
    mRPtr = mLoopBuf;
    mWPtr = mLoopBuf;
    mClientStop = true;
    if (mLoopBuf != NULL) {
        free(mLoopBuf);
        mLoopBuf = NULL;
    }
}

void resetSocketBuffer() {
    mRPtr = mLoopBuf;
    mWPtr = mLoopBuf;
    if (mLoopBuf != NULL) {
        memset(mLoopBuf, 0, LOOP_BUFFER_SIZE);
    }
}

int getSizeBySkt() {
    return getDataSize(mLoopBuf);
}

void getDataBySkt(char *buf, int size) {
    safeRead(mLoopBuf, buf, size);
}

int getInfoBySkt(int type) {
    int ret = -1;
    //ALOGV("[getInfo]type:%d ,mTotal:%d, mStartPts:%d,mType:%d\n", type,mTotal, mStartPts, mType);
    switch (type) {
        case 1:
            ret = mTotal;
            break;
        case 2:
            ret = mStartPts;
            break;
        case 3:
            ret = mType;
            break;
    }
    //ALOGV("[getInfo]type:%d, ret:%d\n", type, ret);
    return ret;
}

void getInSubTypeStrBySkt(char *subTypeS) {
    ALOGV("[getInSubTypeStr]subTypeStr:%s\n",subTypeStr);
    strcpy(subTypeS, subTypeStr);
    memset(subTypeStr, 0, 1024);
}

void getInSubLanStrBySkt(char * subLanS) {
    ALOGV("[getInSubLanStrBySkt]subLanS:%s\n",subLanS);
    strcpy(subLanS, subLanStr);
    memset(subLanStr, 0, 1024);
}

void getPcrscrBySkt(char* pcrStr) {
    //int pcr = mTimeUs/1000*90 + mStartPts;
    int pcr = mTimeUs + mStartPts;
    sprintf(pcrStr, "0x%x", pcr);

    //ALOGV("[getPcrscr]pcr:%x, pcrStr:%s\n",pcr, pcrStr);
}
