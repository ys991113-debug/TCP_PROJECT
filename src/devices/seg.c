// src/devices/seg.c
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include "../../include/device.h"

#define SEG_A 27
#define SEG_B 28
#define SEG_C 29
#define SEG_D 22
#define SEG_E 21
#define SEG_F 24
#define SEG_G 23

// 공통 애노드 - LOW = 켜짐, HIGH = 꺼짐
// 순서: a b c d e f g
int seg_patterns[10][7] = {
    {0,0,0,0,0,0,1}, // 0
    {1,0,0,1,1,1,1}, // 1
    {0,0,1,0,0,1,0}, // 2
    {0,0,0,0,1,1,0}, // 3
    {1,0,0,1,1,0,0}, // 4
    {0,1,0,0,1,0,0}, // 5
    {0,1,0,0,0,0,0}, // 6
    {0,0,0,1,1,1,1}, // 7
    {0,0,0,0,0,0,0}, // 8
    {0,0,0,0,1,0,0}, // 9
};

int seg_pins[7] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};

void seg_display(int num) {
    for(int i = 0; i < 7; i++) {
        digitalWrite(seg_pins[i], seg_patterns[num][i]);
    }
}

void seg_off(void) {
    for(int i = 0; i < 7; i++) {
        digitalWrite(seg_pins[i], HIGH);
    }
}

const char *dev_name(void) { return "SEG"; }

int dev_init(void) {
    for(int i = 0; i < 7; i++) {
        pinMode(seg_pins[i], OUTPUT);
        digitalWrite(seg_pins[i], HIGH);  // 꺼짐
    }
    printf("[SEG] 초기화 완료\n");
    return 0;
}

int dev_handle(const msg_t *msg, char *resp, size_t n) {
    int val = msg->value;

    if(val < 0 || val > 9) {
        snprintf(resp, n,
            "{\"ok\":false,\"cmd\":\"SEG\",\"error\":\"BAD_ARG\",\"message\":\"value must be 0~9\",\"id\":%d}\n",
            msg->id);
        return -1;
    }

    seg_display(val);
    snprintf(resp, n,
        "{\"ok\":true,\"cmd\":\"SEG\",\"data\":{\"value\":%d},\"id\":%d}\n",
        val, msg->id);
    return 0;
}

void dev_cleanup(void) {
    seg_off();
    printf("[SEG] 정리 완료\n");
}