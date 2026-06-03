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
  static int g_sock;
  static int g_id = 1;

  void on_sigint(int s) { (void)s; g_running = 0; }

  void send_request(const char *json) {
      char buf[1024];
      if(send(g_sock, json, strlen(json), 0) == -1) return;
      int n = recv(g_sock, buf, sizeof(buf)-1, 0);
      if(n <= 0) { printf("서버 연결 끊김\n"); g_running=0; return; }
      buf[n] = '\0';
      printf("응답: %s", buf);
      fflush(stdout);
  }

  void print_menu(void) {
      printf("\n========== 원격 장치 제어 ==========\n");
      printf(" 1) LED ON(최대)  2) LED ON(중간)\n");
      printf(" 3) LED ON(최저)  4) LED OFF\n");
      printf(" 5) 조도 읽기\n");
      printf(" 6) 7세그 표시(0~9)\n");
      printf(" 7) 부저 ON       8) 부저 OFF\n");
      printf(" 9) 카운트다운(0~9)\n");
      printf(" 10) AUTO ON     11) AUTO OFF\n");
      printf(" 0) 종료\n");
      printf("====================================\n");
      printf("선택 > ");
      fflush(stdout);
  }

  int main(int argc, char *argv[]) {
      struct sockaddr_in addr;
      int choice;
      char json[256];

      struct sigaction sa = {0};
      sigemptyset(&sa.sa_mask);
      sa.sa_handler = on_sigint;
      sa.sa_flags = 0;
      sigaction(SIGINT, &sa, NULL);
      sa.sa_handler = SIG_IGN;
      sigaction(SIGTSTP, &sa, NULL);
      sigaction(SIGQUIT, &sa, NULL);
      sigaction(SIGHUP,  &sa, NULL);
      sigaction(SIGPIPE, &sa, NULL);

      if(argc != 2) { fprintf(stderr,"usage: %s <IP>\n",argv[0]); exit(1); }
      if((g_sock=socket(AF_INET,SOCK_STREAM,0))==-1) { perror("socket");
  exit(1); }

      addr.sin_family = AF_INET;
      addr.sin_port = htons(PORT);
      addr.sin_addr.s_addr = inet_addr(argv[1]);
      memset(&addr.sin_zero, 0, 8);

      if(connect(g_sock,(struct sockaddr*)&addr,sizeof(addr))==-1) {
          perror("connect"); exit(1);
      }
      printf("서버 연결됨 [%s:%d]\n", argv[1], PORT);

      while(g_running) {
          print_menu();
          if(scanf("%d",&choice)!=1) {
              int c;
              while((c=getchar())!='\n' && c!=EOF);
              continue;
          }
          switch(choice) {
          case 1:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"LED\","
                  "\"args\":{\"state\":\"ON\",\"level\":\"HIGH\"},"
                  "\"id\":%d}",g_id++);
              send_request(json); break;
          case 2:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"LED\","
                  "\"args\":{\"state\":\"ON\",\"level\":\"MID\"},"
                  "\"id\":%d}",g_id++);
              send_request(json); break;
          case 3:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"LED\","
                  "\"args\":{\"state\":\"ON\",\"level\":\"LOW\"},"
                  "\"id\":%d}",g_id++);
              send_request(json); break;
          case 4:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"LED\","
                  "\"args\":{\"state\":\"OFF\"},"
                  "\"id\":%d}",g_id++);
              send_request(json); break;
          case 5:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"LIGHT\","
                  "\"args\":{},\"id\":%d}",g_id++);
              send_request(json); break;
          case 6: {
              int v;
              printf("숫자(0~9) > ");
              fflush(stdout);
              if(scanf("%d",&v)!=1) break;
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"SEG\","
                  "\"args\":{\"value\":%d},\"id\":%d}",v,g_id++);
              send_request(json); break;
          }
          case 7:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"BUZZER\","
                  "\"args\":{\"state\":\"ON\"},\"id\":%d}",g_id++);
              send_request(json); break;
          case 8:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"BUZZER\","
                  "\"args\":{\"state\":\"OFF\"},\"id\":%d}",g_id++);
              send_request(json); break;
          case 9: {
              int v;
              printf("카운트다운(0~9) > ");
              fflush(stdout);
              if(scanf("%d",&v)!=1) break;
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"COUNTDOWN\","
                  "\"args\":{\"value\":%d},\"id\":%d}",v,g_id++);
              send_request(json); break;
          }
          case 10:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"AUTO\","
                  "\"args\":{\"state\":\"ON\"},\"id\":%d}",g_id++);
              send_request(json); break;
          case 11:
              snprintf(json,sizeof(json),
                  "{\"v\":1,\"cmd\":\"AUTO\","
                  "\"args\":{\"state\":\"OFF\"},\"id\":%d}",g_id++);
              send_request(json); break;
          case 0:
              g_running=0; break;
          default:
              printf("잘못된 선택\n"); break;
          }
      }
      printf("\n종료합니다.\n");
      close(g_sock);
      return 0;
  }