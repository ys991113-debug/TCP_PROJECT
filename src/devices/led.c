  #include <stdio.h>
  #include <string.h>
  #include <wiringPi.h>
  extern int softPwmCreate(int,int,int);
extern void softPwmWrite(int,int);
  
  #include "../../include/device.h"

  #define LED 2
  #define PWM_RANGE 100

  const char *dev_name(void) { return "LED"; }

  int dev_init(void) {
      softPwmCreate(LED, 0, PWM_RANGE);
      printf("[LED] 초기화 완료 (softPwm)\n");
      return 0;
  }

  int dev_handle(const msg_t *msg, char *resp, size_t n) {
      if (strcmp(msg->state, "OFF") == 0) {
          softPwmWrite(LED, 0);
          snprintf(resp, n,
              "{\"ok\":true,\"cmd\":\"LED\","
              "\"data\":{\"state\":\"OFF\"},\"id\":%d}\n",
              msg->id);
          return 0;
      }
      if (strcmp(msg->state, "ON") == 0) {
          int brightness = 100;  // 기본 = 최대
          char lv[16];
          strncpy(lv, msg->level, sizeof(lv));

          if (strcmp(lv, "MID") == 0)       brightness = 50;
          else if (strcmp(lv, "LOW") == 0)  brightness = 10;
          else                              brightness = 100; // HIGH or

          softPwmWrite(LED, brightness);

          char bname[8];
          if      (brightness == 100) strcpy(bname, "HIGH");
          else if (brightness == 50)  strcpy(bname, "MID");
          else                        strcpy(bname, "LOW");

          snprintf(resp, n,
              "{\"ok\":true,\"cmd\":\"LED\","
              "\"data\":{\"state\":\"ON\",\"bright\":\"%s\"},\"id\":%d}\n",
              bname, msg->id);
          return 0;
      }
      snprintf(resp, n,
          "{\"ok\":false,\"cmd\":\"LED\","
          "\"error\":\"BAD_ARG\","
          "\"message\":\"state must be ON|OFF\",\"id\":%d}\n",
          msg->id);
      return -1;
  }

  void dev_cleanup(void) {
      softPwmWrite(LED, 0);
      printf("[LED] 정리 완료\n");
  }
