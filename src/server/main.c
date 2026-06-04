#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <semaphore.h>
#include "../../include/device.h"

  #define PORT 1833
  static pthread_mutex_t auto_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t dispatch_mutex = PTHREAD_MUTEX_INITIALIZER;
  static sem_t countdown_sem;

  #define LIGHT_THRESHOLD 180
  static volatile int auto_running = 0;

// loader.c / proto.c 함수 선언
int  load_device(const char *path);
int  dispatch(const msg_t *msg, char *resp, size_t n);
void unload_all(void);
int  parse_request(const char *json, char *cmd, char *state, char *level,int *value,int *id);
  
void on_sigterm(int s) {
      (void)s;
      unload_all();
      remove("/tmp/tcpserver.pid");
      printf("[daemon] 종료\n");
      fflush(stdout);
      exit(0);
  }


  void daemonize(void) {
      pid_t pid = fork();
      if (pid < 0) exit(1);
      if (pid > 0) exit(0);       // 부모 종료

      setsid();                    // 새 세션
      signal(SIGHUP, SIG_IGN);
      chdir("/home/lys/project");                  // 루트로 이동

      // stdin → /dev/null
      int fd = open("/dev/null", O_RDWR);
      dup2(fd, STDIN_FILENO);
      close(fd);

      // stdout/stderr → 로그파일
      int log = open("/tmp/tcpserver.log",
                     O_WRONLY | O_CREAT | O_APPEND, 0644);
      dup2(log, STDOUT_FILENO);
      dup2(log, STDERR_FILENO);
      close(log);

      // PID 파일
      FILE *f = fopen("/tmp/tcpserver.pid", "w");
      if (f) { fprintf(f, "%d\n", getpid()); fclose(f); }

        struct sigaction sa = {0};
        sa.sa_handler = on_sigterm;
        sigaction(SIGTERM, &sa, NULL);

      printf("[daemon] 시작 PID=%d\n", getpid());
      fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
  }
  void *auto_thread(void *arg) {
      char resp[256];
      msg_t m;
      int last = -1;
      while(auto_running) {
          memset(&m, 0, sizeof(m));
          strcpy(m.cmd, "LIGHT");
          dispatch(&m, resp, sizeof(resp));
          int light = 0;
          char *p = strstr(resp, "\"value\":");
          if(p) light = atoi(p + 8);
          printf("[AUTO] light=%d\n", light);
          int want = (light > LIGHT_THRESHOLD) ? 1 : 0;
          if(want != last) {
              memset(&m, 0, sizeof(m));
              strcpy(m.cmd, "LED");
              strcpy(m.state, want ? "ON" : "OFF");
              dispatch(&m, resp, sizeof(resp));
              last = want;
          }
          sleep(1);
      }
      return NULL;
  }
void *countdown_thread(void *arg) {
      int sec = *(int *)arg;
      free(arg);

      char resp[256];
      msg_t m;

      for(int i = sec; i >= 0; i--) {
          memset(&m, 0, sizeof(m));   // 매번 깨끗이
          strcpy(m.cmd, "SEG");
          m.value = i;
          dispatch(&m, resp, sizeof(resp));   // 7세그에 i 표시
          sleep(1);
      }

      // 0 도달 → 부저 ON (학교종)
      memset(&m, 0, sizeof(m));
      strcpy(m.cmd, "BUZZER");
      strcpy(m.state, "ON");
      dispatch(&m, resp, sizeof(resp));
      sem_post(&countdown_sem);
      return NULL;
}


void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buf[4096];
    char resp[4096];
    msg_t msg={0};
    int n;

    while(1) {
        memset(buf, 0, sizeof(buf));
        n = recv(client_fd, buf, sizeof(buf)-1, 0);
        if(n <= 0) {
            printf("클라이언트 연결 끊김\n");
            break;
        }
        buf[n] = '\0';
        printf("수신: %s\n", buf);

        // JSON 파싱
        if(parse_request(buf, msg.cmd, msg.state, msg.level, &msg.value,&msg.id) < 0) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"error\":\"BAD_JSON\",\"id\":0}\n");
            send(client_fd, resp, strlen(resp), 0);
            continue;
        }

        if(strcmp(msg.cmd, "AUTO") == 0) {
             if(strcmp(msg.state, "ON") == 0) {
              pthread_mutex_lock(&auto_mutex);
            if(!auto_running) {
                auto_running = 1;
                pthread_t at;
                pthread_create(&at, NULL, auto_thread, NULL);
                pthread_detach(at);
              }
                pthread_mutex_unlock(&auto_mutex);
                  snprintf(resp, sizeof(resp),
                      "{\"ok\":true,\"cmd\":\"AUTO\","
                      "\"data\":{\"state\":\"ON\"},\"id\":%d}\n", msg.id);
              } else if(strcmp(msg.state, "OFF") == 0) {
                    pthread_mutex_lock(&auto_mutex);
                    auto_running = 0;
                    pthread_mutex_unlock(&auto_mutex);
                  snprintf(resp, sizeof(resp),
                      "{\"ok\":true,\"cmd\":\"AUTO\","
                      "\"data\":{\"state\":\"OFF\"},\"id\":%d}\n", msg.id);
              } else {
                  snprintf(resp, sizeof(resp),
                      "{\"ok\":false,\"cmd\":\"AUTO\","
                      "\"error\":\"BAD_ARG\","
                      "\"message\":\"state must be ON|OFF\","
                      "\"id\":%d}\n", msg.id);
              }
              send(client_fd, resp, strlen(resp), 0);
              continue;
          }

          // COUNTDOWN은 디바이스가 아니라 서버가 직접 처리
        if(strcmp(msg.cmd, "COUNTDOWN") == 0) {
            if(msg.value < 0 || msg.value > 9) {
                snprintf(resp, sizeof(resp),
                    "{\"ok\":false,\"cmd\":\"COUNTDOWN\",\"error\":\"BAD_ARG\","
                    "\"message\":\"value must be 0~9\",\"id\":%d}\n", msg.id);
                send(client_fd, resp, strlen(resp), 0);
                continue;
            }
            if(sem_trywait(&countdown_sem) == 0) {
            int *sec = malloc(sizeof(int));
            *sec = msg.value;
            pthread_t ct;
            pthread_create(&ct, NULL, countdown_thread, sec);
            pthread_detach(ct);
            snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"COUNTDOWN\","
            "\"data\":{\"value\":%d},\"id\":%d}\n",
            msg.value, msg.id);
            } else {
                snprintf(resp, sizeof(resp),
                    "{\"ok\":false,\"cmd\":\"COUNTDOWN\","
                    "\"error\":\"BUSY\","
                    "\"message\":\"카운트다운 진행 중\","
                    "\"id\":%d}\n", msg.id);
            }
              send(client_fd, resp, strlen(resp), 0);
              continue;
          }

        // dispatch → .so 호출
        dispatch(&msg, resp, sizeof(resp));
        send(client_fd, resp, strlen(resp), 0);
    }
    close(client_fd);
    
    return NULL;
}

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    pthread_t tid;

    sem_init(&countdown_sem, 0, 1);
    daemonize();
    wiringPiSetup();
    // .so 로드
    load_device("/home/lys/project/lib/libdev_led.so");
    load_device("/home/lys/project/lib/libdev_light.so");
    load_device("/home/lys/project/lib/libdev_seg.so");
    load_device("/home/lys/project/lib/libdev_buzzer.so");

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket"); exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(server_addr.sin_zero), '\0', 8);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind"); exit(1);
    }

    listen(sockfd, 5);
    printf("서버 시작 → 포트 %d\n", PORT);

    while(1) {
        sin_size = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);
        if(*client_fd == -1) {
            perror("accept");
            free(client_fd);
            continue;
        }
        printf("클라이언트 접속: %s\n", inet_ntoa(client_addr.sin_addr));
        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_detach(tid);
    }

    unload_all();
    close(sockfd);
    return 0;
}
