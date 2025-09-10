#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "expr.h"

static Op op_from(const char *s){
    if (strcmp(s,"=")==0) return OP_EQ;
    if (strcmp(s,">")==0) return OP_GT;
    if (strcmp(s,"<")==0) return OP_LT;
    if (strcmp(s,">=")==0) return OP_GE;
    if (strcmp(s,"<=")==0) return OP_LE;
    return OP_EQ;
}

int parse_predicate(const char *e, Predicate *out){
    // 极简：<col> <op> <rhs>，rhs 可能带引号
    char col[64], op[3], rhs[128];
    if (sscanf(e, " %63s %2[<>=] %127[^\n] ", col, op, rhs) < 3) return -1;
    // 去掉引号与空白
    char *rp = rhs; while(isspace((unsigned char)*rp)) rp++;
    int L = (int)strlen(rp);
    if (L>=2 && rp[0]=='\'' && rp[L-1]=='\''){ rp[L-1]='\0'; rp++; }
    out->col = strdup(col);
    out->op  = op_from(op);
    out->rhs = strdup(rp);
    return 0;
}

int eval_predicate_text(const TableSchema *s, const char *const *vals, const Predicate *p){
    int idx=-1;
    for (int i=0;i<s->ncols;i++) if (strcmp(s->cols[i].name, p->col)==0){ idx=i; break; }
    if (idx<0) return 0;
    if (s->cols[idx].type==T_INT){
        int lv = atoi(vals[idx]), rv = atoi(p->rhs);
        switch(p->op){
            case OP_EQ: return lv==rv;
            case OP_LT: return lv<rv;
            case OP_GT: return lv>rv;
            case OP_LE: return lv<=rv;
            case OP_GE: return lv>=rv;
        }
    }else{
        int cmp = strcmp(vals[idx], p->rhs);
        switch(p->op){
            case OP_EQ: return cmp==0;
            case OP_LT: return cmp<0;
            case OP_GT: return cmp>0;
            case OP_LE: return cmp<=0;
            case OP_GE: return cmp>=0;
        }
    }
    return 0;
}
