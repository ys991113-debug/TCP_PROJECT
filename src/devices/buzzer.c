#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>
#include "../../include/device.h"

#define BUZZER 26

static pthread_t buzzer_tid;
static volatile int buzzer_running = 0;

// 학교종 음계 (Hz)
#define DO  262
#define RE  294
#define MI  330
#define FA  349
#define SOL 392
#define LA  440

// 학교종이 땡땡땡
int melody[] = {SOL,SOL,LA,SOL,SOL,MI, SOL,SOL,MI,MI,RE,
                SOL,SOL,LA,SOL,SOL,MI, SOL,MI,RE,MI,DO, 0};
int duration[]= {4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  8,
                 4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  8, 0};

void tone(int hz, int ms) {
    int period = 1000000 / hz;
    int half = period / 2;
    int cycles = (ms * 1000) / period;
    for(int i = 0; i < cycles && buzzer_running; i++) {
        digitalWrite(BUZZER, HIGH);
        usleep(half);
        digitalWrite(BUZZER, LOW);
        usleep(half);
    }
}

void *buzzer_thread(void *arg) {
    while(buzzer_running) {
        for(int i = 0; melody[i] != 0 && buzzer_running; i++) {
            tone(melody[i], duration[i] * 125);
            usleep(50000);
        }
    }
    digitalWrite(BUZZER, LOW);
    return NULL;
}

const char *dev_name(void) { return "BUZZER"; }

int dev_init(void) {
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
    printf("[BUZZER] 초기화 완료\n");
    return 0;
}

int dev_handle(const msg_t *msg, char *resp, size_t n) {
    if(strcmp(msg->state, "ON") == 0) {
        if(!buzzer_running) {
            buzzer_running = 1;
            pthread_create(&buzzer_tid, NULL, buzzer_thread, NULL);
            pthread_detach(buzzer_tid);
        }
        snprintf(resp, n,
            "{\"ok\":true,\"cmd\":\"BUZZER\",\"data\":{\"state\":\"ON\"},\"id\":%d}\n",
            msg->id);
    } else if(strcmp(msg->state, "OFF") == 0) {
        buzzer_running = 0;
        digitalWrite(BUZZER, LOW);
        snprintf(resp, n,
            "{\"ok\":true,\"cmd\":\"BUZZER\",\"data\":{\"state\":\"OFF\"},\"id\":%d}\n",
            msg->id);
    } else {
        snprintf(resp, n,
            "{\"ok\":false,\"cmd\":\"BUZZER\",\"error\":\"BAD_ARG\",\"message\":\"state must be ON|OFF\",\"id\":%d}\n",
            msg->id);
        return -1;
    }
    return 0;
}

void dev_cleanup(void) {
    buzzer_running = 0;
    digitalWrite(BUZZER, LOW);
    printf("[BUZZER] 정리 완료\n");
}