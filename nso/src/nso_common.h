#ifndef __NSO_COMMON_H__
#define __NSO_COMMON_H__

#include <stdint.h>

typedef uint64_t device_id_t;

static device_id_t* alloc_device_id() {
    device_id_t *id = malloc(sizeof(device_id_t));
    if (!id) {
        LOG_DEBUG("device id alloc failed\n");
        return NULL;
    }
    return id;
}

static void free_device_id(device_id_t *dev_id) {
    if (dev_id) free(dev_id);
}

struct l2addr_s {
    int size;
    uint8_t addr[0];
}__attribute__((packed));

typedef struct l2addr_s l2addr_t;

static l2addr_t* alloc_l2addr(int size) {
    l2addr_t *addr = malloc(sizeof(l2addr_t) + size);
    if (!addr) {
        LOG_DEBUG("l2addr allocation failed\n");
        return NULL;
    }
    addr->size = size;
    return addr;
}

static void free_l2addr(l2addr_t *addr) {
    if (addr) free(addr);
}


#endif
