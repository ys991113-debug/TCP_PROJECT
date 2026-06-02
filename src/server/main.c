#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <pthread.h>
#include "../../include/device.h"

#define PORT 1833

// loader.c / proto.c 함수 선언
int  load_device(const char *path);
int  dispatch(const msg_t *msg, char *resp, size_t n);
void unload_all(void);
int  parse_request(const char *json, char *cmd, char *state, char *level, int *id);

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buf[4096];
    char resp[4096];
    msg_t msg;
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
        if(parse_request(buf, msg.cmd, msg.state, msg.level, &msg.id) < 0) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"error\":\"BAD_JSON\",\"id\":0}\n");
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

    wiringPiSetup();

    // .so 로드
    load_device("./lib/libdev_led.so");

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