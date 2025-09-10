#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "record.h"

static int need_len(const TableSchema *s, const char *const *vals){
    int n = sizeof(RowHeader);
    for (int i=0;i<s->ncols;i++){
        if (s->cols[i].type==T_INT) n += 4;
        else { // varchar
            int L = (int)strlen(vals[i]);
            if (L > s->cols[i].max_len) L = s->cols[i].max_len;
            n += 2 + L; // uint16_t 长度 + 数据
        }
    }
    return n;
}

int serialize_row(const TableSchema *s, const char *const *vals, uint8_t *buf, int cap){
    int need = need_len(s, vals);
    if (!buf) return need;
    if (cap < need) return -1;

    RowHeader *hdr = (RowHeader*)buf;
    hdr->deleted = 0;
    int off = sizeof(RowHeader);

    for (int i=0;i<s->ncols;i++){
        if (s->cols[i].type==T_INT){
            int v = atoi(vals[i]);
            memcpy(buf+off, &v, 4); off+=4;
        }else{
            uint16_t L = (uint16_t)strlen(vals[i]);
            if (L > s->cols[i].max_len) L = s->cols[i].max_len;
            memcpy(buf+off, &L, 2); off+=2;
            memcpy(buf+off, vals[i], L); off+=L;
        }
    }
    return need;
}

int deserialize_row(const TableSchema *s, const uint8_t *buf, int len,
                    char **out, int cap){
    if (cap < s->ncols) return -1;
    if (len < (int)sizeof(RowHeader)) return -2;
    const RowHeader *hdr = (const RowHeader*)buf;
    if (hdr->deleted) return -3;

    int off = sizeof(RowHeader);
    for (int i=0;i<s->ncols;i++){
        if (s->cols[i].type==T_INT){
            if (off+4>len) return -4;
            int v; memcpy(&v, buf+off, 4); off+=4;
            static char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", v);
            out[i] = strdup(tmp);
        }else{
            if (off+2>len) return -5;
            uint16_t L; memcpy(&L, buf+off, 2); off+=2;
            if (off+L>len) return -6;
            out[i] = (char*)malloc(L+1);
            memcpy(out[i], buf+off, L); out[i][L]='\0'; off+=L;
        }
    }
    return 0;
}
