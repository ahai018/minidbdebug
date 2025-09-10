#ifndef PLAN_H
#define PLAN_H

// TODO from teammate: 按你队友的计划格式替换这里
typedef enum {
    NODE_CREATE, NODE_INSERT, NODE_SELECT, NODE_DELETE,
    NODE_SEQSCAN, NODE_FILTER, NODE_PROJECT
} NodeType;

typedef enum { T_INT=1, T_VARCHAR=2 } ColType;

typedef struct {
    const char *table_name;
    int ncols;
    const char **col_names;
    const ColType *col_types;   // 与 columns 表一致
} CreateParams;

typedef struct {
    const char *table_name;
    int ncols;
    const char **col_names;     // 可为空表示“按表定义顺序”
    const char **values_as_text; // 暂以字符串表达，执行时转换
} InsertParams;

typedef struct {
    const char *table_name;
    int nproj;
    const char **proj_cols;     // SELECT 列表
    const char *where_expr;     // 文本表达式：例如 "age > 18"
} SelectParams;

typedef struct {
    const char *table_name;
    const char *where_expr;     // DELETE 的条件
} DeleteParams;

typedef struct PlanNode {
    NodeType type;
    void *params;               // 指向上述 *Params
    struct PlanNode *child;     // SeqScan <- Filter <- Project 之类
} PlanNode;

#endif
