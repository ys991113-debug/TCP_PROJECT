// src/client/client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 1833

static volatile sig_atomic_t g_running = 1;

void on_sigint(int s) {
    g_running = 0;
}

void print_menu() {
    printf("\n========== 원격 장치 제어 ==========\n");
    printf(" 1) LED ON\n");
    printf(" 2) LED OFF\n");
    printf(" 0) 종료\n");
    printf("====================================\n");
    printf("선택 > ");
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[1024];
    int n, choice;

    // 시그널 처리
    struct sigaction sa = {0};
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);   // Ctrl+C → 정상 종료
    signal(SIGQUIT, SIG_IGN);       // Ctrl+\ 무시
    signal(SIGTSTP, SIG_IGN);       // Ctrl+Z 무시
    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);       // 서버 끊겨도 죽지 않음

    if(argc != 2) {
        fprintf(stderr, "usage: %s <서버IP>\n", argv[0]);
        exit(1);
    }

    // 소켓 생성
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(&(server_addr.sin_zero), '\0', 8);

    // 서버 연결
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    printf("서버 연결됨 [%s:%d]\n", argv[1], PORT);

    while(g_running) {
        print_menu();

        if(scanf("%d", &choice) != 1) break;

        memset(buf, 0, sizeof(buf));

        switch(choice) {
            case 1:
                strcpy(buf, "LED ON");
                break;
            case 2:
                strcpy(buf, "LED OFF");
                break;
            case 0:
                strcpy(buf, "quit");
                g_running = 0;
                break;
            default:
                printf("잘못된 선택\n");
                continue;
        }

        // 서버로 전송
        if(send(sockfd, buf, strlen(buf), 0) == -1) {
            perror("send");
            break;
        }

        // 응답 수신
        memset(buf, 0, sizeof(buf));
        n = recv(sockfd, buf, sizeof(buf)-1, 0);
        if(n <= 0) {
            printf("서버 연결 끊김\n");
            break;
        }
        buf[n] = '\0';
        printf("응답: %s\n", buf);
    }

    printf("\n종료합니다.\n");
    close(sockfd);
    return 0;
}