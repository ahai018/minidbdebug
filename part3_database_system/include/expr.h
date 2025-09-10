#ifndef EXPR_H
#define EXPR_H
#include "record.h"

// 仅做最小可用：支持 col =/</>/<=/>= const_int 或 const_string
typedef enum { OP_EQ, OP_LT, OP_GT, OP_LE, OP_GE } Op;

typedef struct {
    const char *col;      // 列名
    Op op;
    const char *rhs;      // 右侧常量(原文本)
} Predicate;

// 解析简单 where 表达式（如 "age > 18" 或 "name = 'Alice'"）
int parse_predicate(const char *expr, Predicate *out);

// 对一条“已反序列化的文本数组”进行判断（真返回1）
int eval_predicate_text(const TableSchema *schema,
                        const char *const *text_values,
                        const Predicate *pred);

#endif
