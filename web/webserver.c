 #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <pthread.h>

  #define HPORT 8080
  #define RIP "100.83.88.7"
  #define RPORT 1833

  int read_light(char *out, int len) {
      int s = socket(AF_INET, SOCK_STREAM, 0);
      if (s < 0) return -1;
      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons(RPORT);
      a.sin_addr.s_addr = inet_addr(RIP);
      memset(&a.sin_zero, 0, 8);
      if (connect(s, (void*)&a, sizeof(a)) < 0) {
          close(s);
          return -1;
      }
      char *q =
          "{\"v\":1,\"cmd\":\"LIGHT\","
          "\"args\":{},\"id\":1}\n";
      send(s, q, strlen(q), 0);
      int n = recv(s, out, len - 1, 0);
      close(s);
      if (n <= 0) return -1;
      out[n] = 0;
      char *nl = strchr(out, '\n');
      if (nl) *nl = 0;
      return 0;
  }

  void send_index(int c) {
      FILE *f = fopen("index.html", "rb");
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
          "Content-Length: %ld\r\n\r\n",
          sz);
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

  void *handle(void *arg) {
      int c = *(int*)arg;
      free(arg);
      char req[2048];
      int n = recv(c, req, sizeof(req)-1, 0);
      if (n <= 0) { close(c); return NULL; }
      req[n] = 0;
      char m[8], p[256];
      sscanf(req, "%7s %255s", m, p);
      if (strcmp(p, "/light") == 0)
          send_light(c);
      else
          send_index(c);
      close(c);
      return NULL;
  }

  int main(void) {
      int s = socket(AF_INET, SOCK_STREAM, 0);
      int opt = 1;
      setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                 &opt, sizeof(opt));
      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons(HPORT);
      a.sin_addr.s_addr = htonl(INADDR_ANY);
      memset(&a.sin_zero, 0, 8);
      if (bind(s, (void*)&a, sizeof(a)) < 0) {
          perror("bind");
          exit(1);
      }
      listen(s, 10);
      printf("web: http://localhost:%d/\n",
             HPORT);
      while (1) {
          int *c = malloc(sizeof(int));
          *c = accept(s, NULL, NULL);
          if (*c < 0) { free(c); continue; }
          pthread_t t;
          pthread_create(&t, NULL, handle, c);
          pthread_detach(t);
      }
      return 0;
  }
