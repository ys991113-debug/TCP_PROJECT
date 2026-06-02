#include <stdio.h>
#include <string.h>
#include <wiringPiI2C.h>
#include "../../include/device.h"

#define I2C_ADDR 0x48
#define CDS_CH   0x00

static int fd = -1;

const char *dev_name(void) { return "LIGHT"; }

int dev_init(void) {
    fd = wiringPiI2CSetupInterface("/dev/i2c-1", I2C_ADDR);
    if(fd < 0) {
        printf("[LIGHT] I2C 초기화 실패\n");
        return -1;
    }
    printf("[LIGHT] 초기화 완료\n");
    return 0;
}

int dev_handle(const msg_t *msg, char *resp, size_t n) {
    // 조도값 읽기
    wiringPiI2CWrite(fd, 0x00 | CDS_CH);
    wiringPiI2CRead(fd);
    int val = wiringPiI2CRead(fd);  // 0~255

    snprintf(resp, n,
        "{\"ok\":true,\"cmd\":\"LIGHT\",\"data\":{\"value\":%d},\"id\":%d}\n",
        val, msg->id);
    return 0;
}

void dev_cleanup(void) {
    printf("[LIGHT] 정리 완료\n");
}