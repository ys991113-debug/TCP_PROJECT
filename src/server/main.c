// src/server/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <pthread.h>


#define PORT 1833
#define LED  2

int parse_request(const char *json, char *cmd, char *state, char *level, int *id);
void build_ok(char *out, size_t n, const char *cmd, const char *data, int id);
void build_err(char *out, size_t n, const char *cmd, const char *error, const char *msg, int id);

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buf[4096];
    char resp[4096];
    char cmd[32], state[16], level[16];
    int id, n;

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
        if(parse_request(buf, cmd, state, level, &id) < 0) {
            build_err(resp, sizeof(resp), "", "BAD_JSON", "JSON 파싱 실패", id);
            send(client_fd, resp, strlen(resp), 0);
            continue;
        }

        // 명령 처리
        if(strcmp(cmd, "LED") == 0) {
            if(strcmp(state, "ON") == 0) {
                digitalWrite(LED, HIGH);
                build_ok(resp, sizeof(resp), "LED", "\"state\":\"ON\"", id);
            } else if(strcmp(state, "OFF") == 0) {
                digitalWrite(LED, LOW);
                build_ok(resp, sizeof(resp), "LED", "\"state\":\"OFF\"", id);
            } else {
                build_err(resp, sizeof(resp), "LED", "BAD_ARG", "state must be ON|OFF", id);
            }
        } else if(strcmp(cmd, "PING") == 0) {
            build_ok(resp, sizeof(resp), "PING", "\"pong\":true", id);
        } else {
            build_err(resp, sizeof(resp), cmd, "UNKNOWN_CMD", "알 수 없는 명령", id);
        }

        send(client_fd, resp, strlen(resp), 0);
    }
    close(client_fd);
    return NULL;
}

int main(void) {
    int sockfd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    pthread_t tid;

    wiringPiSetup();
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

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
        
        int *client_fd = malloc(sizeof(int));  // malloc으로 넘겨야 안전
        *client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);
        if(*client_fd == -1) {
            perror("accept");
            free(client_fd);
            continue;
        }
        printf("클라이언트 접속: %s\n", inet_ntoa(client_addr.sin_addr));

        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_detach(tid);  // join 안하고 바로 다음 accept
    }

    close(sockfd);
    return 0;
}