#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "execution.h"
#include "catalog.h"
#include "storage.h"
#include "page.h"
#include "record.h"
#include "expr.h"

static int do_create(const CreateParams *cp){
    // 1) 分配首个数据页
    page_id_t pid = storage_alloc_page();
    Page p; page_init(&p); storage_write_page(pid, &p);
    // 2) 注册到系统目录
    catalog_register_table(cp->table_name, pid);
    for (int i=0;i<cp->ncols;i++){
        int maxlen = (cp->col_types[i]==T_VARCHAR) ? 255 : 0;
        catalog_add_column(cp->table_name, cp->col_names[i], cp->col_types[i], maxlen);
    }
    printf("OK: table %s created (root=%u)\n", cp->table_name, pid);
    return 0;
}

static int do_insert(const InsertParams *ip){
    TablesRow t; if (catalog_get_table(ip->table_name, &t)!=0){ fprintf(stderr,"table not found\n"); return -1; }
    ColumnDef cols[64]; TableSchema s; if (catalog_load_schema(ip->table_name, &s, cols, 64)!=0){ fprintf(stderr,"schema load fail\n"); return -2; }

    // 将文本值重排为表定义顺序（演示：假设 ip->col_names 为 NULL 或已对齐）
    const char **vals = ip->values_as_text;

    int need = serialize_row(&s, vals, NULL, 0);
    uint8_t *buf = (uint8_t*)malloc(need);
    serialize_row(&s, vals, buf, need);

    Page p; storage_read_page(t.root_pid, &p);
    int slot; if (page_insert(&p, buf, (uint16_t)need, &slot)!=0){ fprintf(stderr,"page full\n"); free(buf); return -3; }
    storage_write_page(t.root_pid, &p);
    free(buf);
    printf("OK: 1 row inserted\n");
    return 0;
}

static int do_select(const SelectParams *sp){
    TablesRow t; if (catalog_get_table(sp->table_name, &t)!=0){ fprintf(stderr,"table not found\n"); return -1; }
    ColumnDef cols[64]; TableSchema s; catalog_load_schema(sp->table_name, &s, cols, 64);

    Predicate pred; int has_pred=0;
    if (sp->where_expr && sp->where_expr[0]) has_pred = (parse_predicate(sp->where_expr, &pred)==0);

    // 简化：只扫 root_pid 这一页（可扩展为多页链）
    Page p; storage_read_page(t.root_pid, &p);
    // 打印表头
    for (int i=0;i<s.ncols;i++) printf("%s%s", s.cols[i].name, (i+1<s.ncols)?"\t":"\n");
    // 逐槽读取
    for (int i=0;;i++){
        const void *rec; uint16_t len;
        if (page_get(&p, i, &rec, &len)!=0) break;
        // 反序列化为文本
        char *txt[64]; if (deserialize_row(&s, rec, len, txt, 64)!=0) continue;
        int ok = 1;
        if (has_pred) ok = eval_predicate_text(&s, (const char* const*)txt, &pred);
        if (ok){
            // 简化：不做投影，直接全列输出（可根据 sp->proj_cols 做子集输出）
            for (int c=0;c<s.ncols;c++){
                printf("%s%s", txt[c], (c+1<s.ncols)?"\t":"\n");
                free(txt[c]);
            }
        }else{
            for (int c=0;c<s.ncols;c++) free(txt[c]);
        }
    }
    if (has_pred){ free((void*)pred.col); free((void*)pred.rhs); }
    return 0;
}

static int do_delete(const DeleteParams *dp){
    TablesRow t; if (catalog_get_table(dp->table_name, &t)!=0){ fprintf(stderr,"table not found\n"); return -1; }
    ColumnDef cols[64]; TableSchema s; catalog_load_schema(dp->table_name, &s, cols, 64);

    Predicate pred; int has_pred = (dp->where_expr && dp->where_expr[0] && parse_predicate(dp->where_expr,&pred)==0);

    Page p; storage_read_page(t.root_pid, &p);
    int affected=0;
    for (int i=0;;i++){
        const void *rec; uint16_t len;
        if (page_get(&p, i, &rec, &len)!=0) break;
        char *txt[64]; if (deserialize_row(&s, rec, len, txt, 64)!=0) continue;
        int ok = has_pred ? eval_predicate_text(&s, (const char* const*)txt, &pred) : 1;
        for (int c=0;c<s.ncols;c++) free(txt[c]);
        if (ok){ page_delete(&p, i); affected++; }
    }
    storage_write_page(t.root_pid, &p);
    if (has_pred){ free((void*)pred.col); free((void*)pred.rhs); }
    printf("OK: %d row(s) deleted\n", affected);
    return 0;
}

int execute_plan(const PlanNode *plan){
    switch(plan->type){
        case NODE_CREATE: return do_create((const CreateParams*)plan->params);
        case NODE_INSERT: return do_insert((const InsertParams*)plan->params);
        case NODE_SELECT: return do_select((const SelectParams*)plan->params);
        case NODE_DELETE: return do_delete((const DeleteParams*)plan->params);
        default: fprintf(stderr,"unsupported node\n"); return -99;
    }
}
