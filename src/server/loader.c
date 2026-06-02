// src/server/loader.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "../../include/device.h"

#define MAX_DEVICES 10

typedef struct {
    void        *handle;
    const char *(*dev_name)(void);
    int         (*dev_init)(void);
    int         (*dev_handle)(const msg_t *msg, char *resp, size_t n);
    void        (*dev_cleanup)(void);
} device_entry_t;

static device_entry_t devices[MAX_DEVICES];
static int dev_count = 0;

int load_device(const char *path) {
    void *handle = dlopen(path, RTLD_LAZY);
    if(!handle) {
        fprintf(stderr, "dlopen 실패: %s\n", dlerror());
        return -1;
    }

    device_entry_t e;
    e.handle      = handle;
    e.dev_name    = dlsym(handle, "dev_name");
    e.dev_init    = dlsym(handle, "dev_init");
    e.dev_handle  = dlsym(handle, "dev_handle");
    e.dev_cleanup = dlsym(handle, "dev_cleanup");

    if(!e.dev_name || !e.dev_init || !e.dev_handle || !e.dev_cleanup) {
        fprintf(stderr, "dlsym 실패: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    e.dev_init();
    printf("[loader] %s 로드 완료\n", e.dev_name());
    devices[dev_count++] = e;
    return 0;
}

// cmd로 디바이스 찾아서 handle 호출
int dispatch(const msg_t *msg, char *resp, size_t n) {
    for(int i = 0; i < dev_count; i++) {
        if(strcmp(devices[i].dev_name(), msg->cmd) == 0) {
            return devices[i].dev_handle(msg, resp, n);
        }
    }
    snprintf(resp, n,
        "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"UNKNOWN_CMD\",\"id\":%d}\n",
        msg->cmd, msg->id);
    return -1;
}

void unload_all(void) {
    for(int i = 0; i < dev_count; i++) {
        devices[i].dev_cleanup();
        dlclose(devices[i].handle);
    }
    dev_count = 0;
}