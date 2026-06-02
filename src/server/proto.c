// src/server/proto.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/jsmn.h"

// 토큰에서 문자열 추출
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if(tok->type == JSMN_STRING &&
       (int)strlen(s) == tok->end - tok->start &&
       strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

// 토큰 값 복사
void json_get_str(const char *json, jsmntok_t *tok, char *out, size_t n) {
    int len = tok->end - tok->start;
    if(len >= (int)n) len = n - 1;
    strncpy(out, json + tok->start, len);
    out[len] = '\0';
}

// JSON 파싱 → cmd, state 추출
int parse_request(const char *json, char *cmd, char *state, char *level, int *id) {
    jsmn_parser p;
    jsmntok_t tokens[32];

    jsmn_init(&p);
    int r = jsmn_parse(&p, json, strlen(json), tokens, 32);
    if(r < 0) return -1;

    *id = 0;
    cmd[0] = '\0';
    state[0] = '\0';
    if(level) level[0] = '\0';

    for(int i = 1; i < r; i++) {
        if(jsoneq(json, &tokens[i], "cmd") == 0) {
            json_get_str(json, &tokens[i+1], cmd, 32);
            i++;
        } else if(jsoneq(json, &tokens[i], "id") == 0) {
            char tmp[16];
            json_get_str(json, &tokens[i+1], tmp, 16);
            *id = atoi(tmp);
            i++;
        } else if(jsoneq(json, &tokens[i], "state") == 0) {
            json_get_str(json, &tokens[i+1], state, 16);
            i++;
        } else if(jsoneq(json, &tokens[i], "level") == 0 && level) {
            json_get_str(json, &tokens[i+1], level, 16);
            i++;
        }
    }
    return 0;
}

// 성공 응답 생성
void build_ok(char *out, size_t n, const char *cmd, const char *data, int id) {
    snprintf(out, n,
        "{\"ok\":true,\"cmd\":\"%s\",\"data\":{%s},\"id\":%d}\n",
        cmd, data, id);
}

// 실패 응답 생성
void build_err(char *out, size_t n, const char *cmd, const char *error, const char *msg, int id) {
    snprintf(out, n,
        "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"%s\",\"message\":\"%s\",\"id\":%d}\n",
        cmd, error, msg, id);
}