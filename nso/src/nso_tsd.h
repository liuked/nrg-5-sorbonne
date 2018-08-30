#ifndef __NSO_TSD_H__
#define __NSO_TSD_H__

typedef struct {
    //the index of interface which is used to broadcast beacons
    int if_index;
}nso_tsd_t;

int tsd_init();
int tsd_broadcast_beacons();
int tsd_ack_registration();

#endif
