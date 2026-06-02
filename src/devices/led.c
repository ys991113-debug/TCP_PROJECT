// src/devices/led.c
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include "../../include/device.h"

#define LED 2

const char *dev_name(void) { return "LED"; }

int dev_init(void) {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    printf("[LED] 초기화 완료\n");
    return 0;
}

int dev_handle(const msg_t *msg, char *resp, size_t n) {
    if(strcmp(msg->state, "ON") == 0) {
        digitalWrite(LED, HIGH);
        snprintf(resp, n,
            "{\"ok\":true,\"cmd\":\"LED\",\"data\":{\"state\":\"ON\"},\"id\":%d}\n",
            msg->id);
    } else if(strcmp(msg->state, "OFF") == 0) {
        digitalWrite(LED, LOW);
        snprintf(resp, n,
            "{\"ok\":true,\"cmd\":\"LED\",\"data\":{\"state\":\"OFF\"},\"id\":%d}\n",
            msg->id);
    } else {
        snprintf(resp, n,
            "{\"ok\":false,\"cmd\":\"LED\",\"error\":\"BAD_ARG\",\"message\":\"state must be ON|OFF\",\"id\":%d}\n",
            msg->id);
        return -1;
    }
    return 0;
}

void dev_cleanup(void) {
    digitalWrite(LED, LOW);
    printf("[LED] 정리 완료\n");
}