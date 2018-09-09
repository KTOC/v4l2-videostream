#ifndef _V4L2_H_
#define _V4L2_H_

void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
int read_frame(void);
unsigned char *snapFrame();
void stop_capturing(void);
void start_capturing(void);
void uninit_device(void);
void init_read(unsigned int buffer_size);
void init_mmap(void);
void init_userp(unsigned int buffer_size);
struct v4l2_format init_device(void);
void close_device(void);
void open_device(void);
void list_formats();
void buffer_export(int v4l2fd, int index, int *dmafd);
#endif
