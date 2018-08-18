#ifndef __NIC_OPS_INTF_H__
#define __NIC_OPS_INTF_H__

#include "packet.h"
#include "log.h"
#include <stdint.h>

struct nic_info_s {
    int mtu;
    //TODO: add more 
};

typedef struct nic_info_s nic_info_t;

typedef void nic_handle_t;

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
    free(addr);
}


struct nic_ops_s {
    int (*open)(char *name, nic_handle_t *handle);
    int (*close)(nic_handle_t *handle);
    int (*send)(nic_handle_t *handle, packet_t *p, l2addr_t *dst);
    int (*receive)(nic_handle_t *handle, packet_t *p, l2addr_t **src, l2addr_t **dst);
    int (*get_info)(nic_handle_t *handle, nic_info_t *info);
};

typedef struct nic_ops_s nic_ops_t;

#endif
