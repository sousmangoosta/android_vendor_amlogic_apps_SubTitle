#ifndef _SUB_CONTROL_H_
#define _SUB_CONTROL_H_

#ifdef  __cplusplus
extern "C" {
#endif

int subtitle_poll_sub_fd(int sub_fd, int timeout);
int subtitle_get_sub_size_fd(int sub_fd);
int subtitle_read_sub_data_fd(int sub_fd, char *buf, int length);
int update_read_pointer(int sub_handle, int flag);

#ifdef  __cplusplus
}
#endif

#endif
