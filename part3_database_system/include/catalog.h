#ifndef CATALOG_H
#define CATALOG_H
#include <stdint.h>
#include "record.h"

// 系统表：tables(name TEXT PK, root_page_id INT)
typedef struct {
    char name[32];
    uint32_t root_pid;     // 表数据的“起始页”（或元信息页）
} TablesRow;

// 系统表：columns(table_name TEXT, col_name TEXT, col_type INT, max_len INT)
typedef struct {
    char table_name[32];
    char col_name[32];
    int  col_type;
    int  max_len;
} ColumnsRow;

int catalog_bootstrap();   // 初始化catalog（若不存在则创建系统表）
int catalog_register_table(const char *name, uint32_t root_pid);
int catalog_add_column(const char *t, const char *c, int type, int max_len);

// 查询
int catalog_get_table(const char *name, TablesRow *out);
int catalog_load_schema(const char *name, TableSchema *schema, ColumnDef *cols_buf, int cols_cap);

// 系统表名字（常量）
extern const char *CAT_TABLES;
extern const char *CAT_COLUMNS;

#endif
