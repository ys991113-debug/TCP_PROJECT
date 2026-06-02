// include/device.h
#ifndef DEVICE_H
#define DEVICE_H

#include <stddef.h>

typedef struct {
    char cmd[32];
    char state[16];
    char level[16];
    int  value;
    int  id;
} msg_t;

// 모든 .so가 노출하는 공통 인터페이스
int         dev_init(void);
int         dev_handle(const msg_t *msg, char *resp, size_t n);
void        dev_cleanup(void);
const char *dev_name(void);

#endif