#ifndef RECORD_H
#define RECORD_H
#include <stdint.h>
#include "plan.h"

typedef struct {
    const char *name;
    ColType type;
    uint16_t max_len; // 对 VARCHAR 生效
} ColumnDef;

typedef struct {
    const char *table_name;
    int ncols;
    ColumnDef *cols;
} TableSchema;

// 行头：简化处理
typedef struct {
    uint8_t deleted; // 0:正常 1:已删（便于跨页检索时快速判断）
} RowHeader;

// 将 InsertParams/文本值 按 schema 转为二进制记录
// 返回写入到 buf 的字节数；若 buf==NULL 先返回需要的长度
int serialize_row(const TableSchema *schema,
                  const char *const *values_as_text,
                  uint8_t *buf, int buf_cap);

// 反序列化：从二进制记录解析各列（演示：转成文本数组回显）
int deserialize_row(const TableSchema *schema,
                    const uint8_t *buf, int len,
                    char **out_text_values, int out_cap);

#endif
