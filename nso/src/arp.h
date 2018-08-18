#ifndef __ARP_H__
#define __ARP_H__

#include "hash_crc.h"
#include "list.h"
#include "nso_common.h"
#include <stdint.h>

typedef enum {
    ARP_ACTIVE = 0,
    ARP_EXPIRED = 1,
}arp_entry_status_e;

typedef struct {
    device_id_t *dev_id;
    l2addr_t *l2addr;

    struct list_head ll;
    struct hlist_node hl_devid;
    struct hlist_node hl_l2addr;

    arp_entry_status_e status;

}arp_entry_t;

static arp_entry_t* alloc_arp_entry(device_id_t *dev_id, l2addr_t *l2addr) {
    arp_entry_t *e = malloc(sizeof(arp_entry_t));
    if (!e) {
        LOG_DEBUG("arp entry alloc failed\n");
        return NULL;
    }
    e->dev_id = dev_id;
    e->l2addr = l2addr;
    e->status = ARP_ACTIVE;
    //TODO: initialize table pointers
    
    return e;
}

static void free_arp_entry(arp_entry_t *e) {
    if (e) {
        free_device_id(e->dev_id);
        free_l2addr(e->l2addr);
        free(e);
    }
}

struct arp_table_s {
    int size;
    //used to store (device_id, l2addr) tuple in a linear list
    struct list_head ll_head;
    //hash table which uses device_id as the key
    //hash table which uses l2addr as the key
    //lock?
};

typedef struct arp_table_s arp_table_t;

//operation
//create
//add new entry
//lookup entry from dev_id
//lookup entry from l2addr
//del entry
//aging

#endif
