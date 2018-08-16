#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdlib.h>
#include <string.h>

struct packet_s {
    //size means the total size of buf
    int size;
    //byte_len is the current used size of buf
    int byte_len;
    char buf[0];
}__attribute__((packed));

typedef struct packet_s packet_t;

static packet_t* alloc_packet(int size) {
    packet_t *p = malloc(sizeof(packet_t) + size);
    if (!p) {
        return NULL;
    }
    memset(p, 0, sizeof(packet_t) + size);
    p->byte_len = 0;
    p->size = size;
    return p;
}

static void free_packet(packet_t *p) {
    if(p) free(p);
}

#endif
