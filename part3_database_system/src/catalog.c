#include <stdio.h>
#include <string.h>
#include "catalog.h"
#include "storage.h"
#include "page.h"
#include "record.h"

const char *CAT_TABLES  = "_tables";
const char *CAT_COLUMNS = "_columns";

// 演示：固定两页ID(真实实现：放到超级块/头页)
static const uint32_t TABLES_PID  = 1;
static const uint32_t COLUMNS_PID = 2;

static int ensure_page(uint32_t pid){
    Page p;
    if (storage_read_page(pid, &p)!=0){
        page_init(&p);
        return storage_write_page(pid, &p);
    }
    return 0;
}

int catalog_bootstrap(){
    // 确保系统表页可用
    if (ensure_page(TABLES_PID)!=0) return -1;
    if (ensure_page(COLUMNS_PID)!=0) return -2;
    return 0;
}

int catalog_register_table(const char *name, uint32_t root_pid){
    Page p; storage_read_page(TABLES_PID, &p);
    // 行 = RowHeader + name(varchar32) + root_pid(int)
    const char *vals[2] = { name, NULL };
    TableSchema s = { CAT_TABLES, 2, NULL };
    ColumnDef cols[2] = { {"name", T_VARCHAR, 31}, {"root_pid", T_INT, 0} };
    s.cols = cols;
    int need = serialize_row(&s, vals, NULL, 0);
    uint8_t buf[128]; serialize_row(&s, vals, buf, sizeof(buf));
    memcpy(buf+sizeof(RowHeader)+2+strlen(name), &root_pid, 4);
    int slot; page_insert(&p, buf, need, &slot);
    storage_write_page(TABLES_PID, &p);
    return 0;
}

int catalog_add_column(const char *t, const char *c, int type, int max_len){
    Page p; storage_read_page(COLUMNS_PID, &p);
    const char *vals3[3] = { t, c, NULL };
    TableSchema s = { CAT_COLUMNS, 4, NULL };
    ColumnDef cols[4] = {
        {"table_name", T_VARCHAR, 31},
        {"col_name",   T_VARCHAR, 31},
        {"col_type",   T_INT,     0 },
        {"max_len",    T_INT,     0 }
    };
    s.cols = cols;
    int need = serialize_row(&s, vals3, NULL, 0) + 8; // 后面手工写两个 int
    uint8_t buf[128]; serialize_row(&s, vals3, buf, sizeof(buf));
    int off = sizeof(RowHeader);
    // 跳过两段 varchar
    for (int k=0;k<2;k++){ uint16_t L; memcpy(&L, buf+off, 2); off+=2+L; }
    memcpy(buf+off, &type, 4); off+=4;
    memcpy(buf+off, &max_len, 4);
    int slot; page_insert(&p, buf, need, &slot);
    storage_write_page(COLUMNS_PID, &p);
    return 0;
}

int catalog_get_table(const char *name, TablesRow *out){
    Page p; storage_read_page(TABLES_PID, &p);
    // 线性扫（系统表很小）
    for (int i=0;;i++){
        const void *rec; uint16_t len;
        if (page_get(&p,i,&rec,&len)!=0) break;
        // 解析
        TableSchema s={CAT_TABLES,2,NULL};
        ColumnDef cols[2]={{"name",T_VARCHAR,31},{"root_pid",T_INT,0}}; s.cols=cols;
        char *vals[2]; if (deserialize_row(&s, rec, len, vals, 2)==0){
            if (strcmp(vals[0], name)==0){
                strncpy(out->name, vals[0], 31);
                memcpy(&out->root_pid, ((uint8_t*)rec)+sizeof(RowHeader)+2+strlen(vals[0]), 4);
                out->name[31]='\0';
                free(vals[0]); free(vals[1]);
                return 0;
            }
            free(vals[0]); free(vals[1]);
        }
    }
    return -1;
}

int catalog_load_schema(const char *t, TableSchema *schema, ColumnDef *buf, int cap){
    // 扫描 _columns 取出 t 的列定义
    Page p; storage_read_page(COLUMNS_PID, &p);
    int n=0;
    for (int i=0;;i++){
        const void *rec; uint16_t len;
        if (page_get(&p,i,&rec,&len)!=0) break;
        TableSchema s={CAT_COLUMNS,4,NULL};
        ColumnDef cols[4]={{"table_name",T_VARCHAR,31},{"col_name",T_VARCHAR,31},{"col_type",T_INT,0},{"max_len",T_INT,0}};
        s.cols=cols;
        char *vals[4]; if (deserialize_row(&s, rec, len, vals, 4)==0){
            if (strcmp(vals[0], t)==0 && n<cap){
                buf[n].name = strdup(vals[1]);
                buf[n].type = (ColType)atoi(vals[2]);
                buf[n].max_len = atoi(vals[3]);
                n++;
            }
            for(int k=0;k<4;k++) free(vals[k]);
        }
    }
    schema->table_name = t;
    schema->ncols = n;
    schema->cols = buf;
    return (n>0)?0:-1;
}
