#ifndef PAGE_H
#define PAGE_H
#include <stdint.h>
#include "storage.h"

typedef struct {
    uint16_t free_start;  // 自由空间起点，从页头向下增长
    uint16_t slot_count;  // 槽数量
    uint16_t free_bytes;  // 剩余自由空间（含碎片）
    uint16_t reserved;    // 对齐
} PageHeader;

// 槽条目：偏移与长度，length=0 表示该槽已删除(逻辑删除)
typedef struct {
    uint16_t off;
    uint16_t len;
} Slot;

// API
void page_init(Page *p);
int  page_insert(Page *p, const void *rec, uint16_t len, int *slot_id_out);
int  page_get    (const Page *p, int slot_id, const void **rec, uint16_t *len);
int  page_delete (Page *p, int slot_id);   // 逻辑删除
int  page_compact(Page *p);                // 整理碎片（可选）

#endif
