#ifndef STORAGE_H
#define STORAGE_H
#include <stddef.h>
#include <stdint.h>

// TODO from teammate: 替换为真实底层接口 (缓存/磁盘/页分配)
// 这里先给“最小可用原型”
typedef uint32_t page_id_t;

#define PAGE_SIZE 4096

typedef struct {
    uint8_t data[PAGE_SIZE];
} Page;

page_id_t storage_alloc_page();                 // 分配新页
int       storage_free_page(page_id_t pid);     // 释放页
int       storage_read_page(page_id_t pid, Page *out);
int       storage_write_page(page_id_t pid, const Page *in);

#endif
