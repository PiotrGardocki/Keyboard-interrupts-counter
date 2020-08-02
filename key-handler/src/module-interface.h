#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

#include <linux/ioctl.h>

#define MAX_IO_BUFFER 40

#define MAJOR_NUMBER 240
#define DEVICE_NAME "key-handler"

#define QUERY_GET_RESET_COUNT _IOR(MAJOR_NUMBER, 1, char *)
#define QUERY_GET_RESET_DATE  _IOR(MAJOR_NUMBER, 2, char *)
#define QUERY_RESET_COUNTER   _IO(MAJOR_NUMBER, 3)

#endif
