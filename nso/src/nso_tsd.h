#ifndef __NSO_TSD_H__
#define __NSO_TSD_H__

#include <stdint.h>

typedef enum {
    NSG_TSD_MSG_REG = 0,
    NSG_TSD_MSG_REG_REPLY,
    NSG_TSD_MSG_MAX,
};

typedef struct {
    //the index of interface which is used to broadcast beacons
    int if_index;
}nso_tsd_t;

int tsd_init();
int tsd_broadcast_beacons();
int tsd_process_rx(uint8_t *data, int size);

#endif
