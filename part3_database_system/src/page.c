#include <string.h>
#include "page.h"

#define HDR(p) ((PageHeader*)(p->data))
#define SLOTS_PTR(p) ((Slot*)((uint8_t*)p->data + PAGE_SIZE) - HDR(p)->slot_count)

void page_init(Page *p){
    memset(p->data, 0, PAGE_SIZE);
    HDR(p)->free_start = sizeof(PageHeader);
    HDR(p)->slot_count = 0;
    HDR(p)->free_bytes = PAGE_SIZE - sizeof(PageHeader);
}

static int ensure_slot(Page *p){
    // 需要扩一条槽：从页尾向上分配
    uint16_t need = sizeof(Slot);
    if (HDR(p)->free_bytes < need) return -1;
    HDR(p)->free_bytes -= need;
    HDR(p)->slot_count += 1;
    return HDR(p)->slot_count - 1;
}

int page_insert(Page *p, const void *rec, uint16_t len, int *slot_id_out){
    // 需要空间：len + 一个槽
    uint16_t need = len + sizeof(Slot);
    if (HDR(p)->free_bytes < need) return -1;

    int slot_id = ensure_slot(p);
    if (slot_id < 0) return -1;

    // 记录从 free_start 位置写入
    memcpy(p->data + HDR(p)->free_start, rec, len);
    Slot *slots = SLOTS_PTR(p);
    slots[slot_id].off = HDR(p)->free_start;
    slots[slot_id].len = len;

    HDR(p)->free_start += len;
    HDR(p)->free_bytes -= len;
    if (slot_id_out) *slot_id_out = slot_id;
    return 0;
}

int page_get(const Page *p, int slot_id, const void **rec, uint16_t *len){
    if (slot_id < 0 || slot_id >= HDR(p)->slot_count) return -1;
    const Slot *slots = SLOTS_PTR((Page*)p);
    if (slots[slot_id].len == 0) return -2; // deleted
    *rec = p->data + slots[slot_id].off;
    *len = slots[slot_id].len;
    return 0;
}

int page_delete(Page *p, int slot_id){
    if (slot_id < 0 || slot_id >= HDR(p)->slot_count) return -1;
    Slot *slots = SLOTS_PTR(p);
    slots[slot_id].len = 0; // 逻辑删除
    return 0;
}

int page_compact(Page *p){ (void)p; return 0; } // 可选实现
