  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/epoll.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>

  static const char *RIP = "100.83.88.7";

  #define HPORT 8080
  #define RPORT 1833
  #define MAX_EVENTS 64

  int set_nonblocking(int fd) {
      int flags = fcntl(fd, F_GETFL, 0);
      return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  int read_light(char *out, int len) {
      int s = socket(AF_INET, SOCK_STREAM, 0);
      if (s < 0) return -1;
      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons(RPORT);
      a.sin_addr.s_addr = inet_addr(RIP);
      memset(&a.sin_zero, 0, 8);
      if (connect(s, (void*)&a, sizeof(a)) < 0) {
          close(s); return -1;
      }
      char *q =
          "{\"v\":1,\"cmd\":\"LIGHT\","
          "\"args\":{},\"id\":1}\n";
      send(s, q, strlen(q), 0);
      int n = recv(s, out, len-1, 0);
      close(s);
      if (n <= 0) return -1;
      out[n] = 0;
      char *nl = strchr(out, '\n');
      if (nl) *nl = 0;
      return 0;
  }

  void send_index(int c) {
      FILE *f = fopen("web/index.html", "rb");
      if (!f) return;
      fseek(f, 0, SEEK_END);
      long sz = ftell(f);
      fseek(f, 0, SEEK_SET);
      char *b = malloc(sz);
      fread(b, 1, sz, f);
      fclose(f);
      char h[128];
      sprintf(h,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "Content-Length: %ld\r\n\r\n", sz);
      send(c, h, strlen(h), 0);
      send(c, b, sz, 0);
      free(b);
  }

  void send_light(int c) {
      char v[512], body[512], r[1100];
      if (read_light(v, sizeof(v)) != 0)
          strcpy(v, "{\"ok\":false}");
      sprintf(body, "%s", v);
      sprintf(r,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: %d\r\n\r\n%s",
          (int)strlen(body), body);
      send(c, r, strlen(r), 0);
  }

 void send_cmd(int c, const char *json) {
      char err[] =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: 12\r\n\r\n"
          "{\"ok\":false}";
      int s = socket(AF_INET, SOCK_STREAM, 0);
      if (s < 0) { send(c, err, strlen(err), 0); return; }
      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons(RPORT);
      a.sin_addr.s_addr = inet_addr(RIP);
      memset(&a.sin_zero, 0, 8);
      if (connect(s, (void*)&a, sizeof(a)) < 0) {
          close(s); send(c, err, strlen(err), 0); return;
      }
      send(s, json, strlen(json), 0);
      char v[512]; int n = recv(s, v, sizeof(v)-1, 0);
      close(s);
      if (n <= 0) { send(c, err, strlen(err), 0); return; }
      v[n] = 0;
      char *nl = strchr(v, '\n'); if (nl) *nl = 0;
      char r[1100];
      sprintf(r,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: %d\r\n\r\n%s",
          (int)strlen(v), v);
      send(c, r, strlen(r), 0);
  }

  void handle_client(int c) {
      char req[4096];
      int n = recv(c, req, sizeof(req)-1, 0);
      if (n <= 0) return;
      req[n] = 0;
      char m[8], p[256];
      sscanf(req, "%7s %255s", m, p);

      if (strcmp(p, "/light") == 0) {
          send_light(c);
      } else if (strcmp(p, "/led/on/high") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"LED\","
              "\"args\":{\"state\":\"ON\",\"level\":\"HIGH\"},\"id\":1}\n");
      } else if (strcmp(p, "/led/on/mid") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"LED\","
              "\"args\":{\"state\":\"ON\",\"level\":\"MID\"},\"id\":1}\n");
      } else if (strcmp(p, "/led/on/low") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"LED\","
              "\"args\":{\"state\":\"ON\",\"level\":\"LOW\"},\"id\":1}\n");
      } else if (strcmp(p, "/led/off") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"LED\","
              "\"args\":{\"state\":\"OFF\"},\"id\":1}\n");
      } else if (strcmp(p, "/buzzer/on") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"BUZZER\","
              "\"args\":{\"state\":\"ON\"},\"id\":1}\n");
      } else if (strcmp(p, "/buzzer/off") == 0) {
          send_cmd(c,
              "{\"v\":1,\"cmd\":\"BUZZER\","
              "\"args\":{\"state\":\"OFF\"},\"id\":1}\n");
      } else if (strncmp(p, "/countdown/", 11) == 0) {
          int v = atoi(p + 11);
          if (v >= 0 && v <= 9) {
              char json[128];
              sprintf(json,
                  "{\"v\":1,\"cmd\":\"COUNTDOWN\","
                  "\"args\":{\"value\":%d},\"id\":1}\n", v);
              send_cmd(c, json);
          }
      } else if (strncmp(p, "/seg/", 5) == 0) {
          int v = atoi(p + 5);
          if (v >= 0 && v <= 9) {
              char json[128];
              sprintf(json,
                  "{\"v\":1,\"cmd\":\"SEG\","
                  "\"args\":{\"value\":%d},\"id\":1}\n", v);
              send_cmd(c, json);
          }
    } else if(strcmp(p, "/auto/on") == 0){
         send_cmd(c,
          "{\"v\":1,\"cmd\":\"AUTO\","
          "\"args\":{\"state\":\"ON\"},\"id\":1}\n");
        } else if (strcmp(p, "/auto/off") == 0) {
        send_cmd(c,
        "{\"v\":1,\"cmd\":\"AUTO\","
        "\"args\":{\"state\":\"OFF\"},\"id\":1}\n");

      }else {
          send_index(c);
      }
  }

  int main(void) {

   
      int sfd = socket(AF_INET, SOCK_STREAM, 0);
      int opt = 1;
      setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      set_nonblocking(sfd);

      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons(HPORT);
      a.sin_addr.s_addr = inet_addr(RIP);
      memset(&a.sin_zero, 0, 8);

      if (bind(sfd, (void*)&a, sizeof(a)) < 0) {
          perror("bind"); exit(1);
      }
      listen(sfd, 10);

      int epfd = epoll_create1(0);
      struct epoll_event ev, evs[MAX_EVENTS];
      ev.events = EPOLLIN;
      ev.data.fd = sfd;
      epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

      printf("epoll 웹서버 시작 -> http://%s:%d/\n", RIP, HPORT);
      fflush(stdout);

      while (1) {
          int n = epoll_wait(epfd, evs, MAX_EVENTS, -1);
          for (int i = 0; i < n; i++) {
              if (evs[i].data.fd == sfd) {
                  int cfd = accept(sfd, NULL, NULL);
                  if (cfd < 0) continue;
                  set_nonblocking(cfd);
                  ev.events = EPOLLIN;
                  ev.data.fd = cfd;
                  epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
              } else {
                  int cfd = evs[i].data.fd;
                  handle_client(cfd);
                  epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                  close(cfd);
              }
          }
      }
      close(sfd);
      close(epfd);
      return 0;
  }