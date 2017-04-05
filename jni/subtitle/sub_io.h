#ifndef SUB_IO_H
#define SUB_IO_H

#define TYPE_TOTAL          1
#define TYPE_STARTPTS       2
#define TYPE_SUBTYPE        3

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    IO_TYPE_DEV = 0,
    IO_TYPE_SOCKET = 1,
    IO_TYPE_END = 0xFF,
} IOType;

IOType mIOType;

//for dev
int pollFd(int sub_fd, int timeout);

//for socket
void startSocketServer();
void stopSocketServer();
int getInfo(int type);

// for common
int getSize(int sub_fd);
void getData(int sub_fd, char *buf, int size);
void setIOType(IOType type);
IOType getIOType();
void getPcrscr(char* pcrStr);

#ifdef  __cplusplus
}
#endif

#endif
