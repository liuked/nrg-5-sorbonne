#ifndef __NSO_TSD_H__
#define __NSO_TSD_H__

#define NSO_TSD_MSG_BASE 0
#define NSO_TSD_MSG_REG NSO_TSD_MSG_BASE
#define NSO_TSD_MSG_REPLY (NSO_TSD_MSG_REG + 1)
#define NSO_TSD_MSG_MAX (NSO_TSD_MSG_REPLY + 1)

typedef struct {
    //the index of interface which is used to broadcast beacons
    int if_index;
}nso_tsd_t;

int tsd_init();
int tsd_broadcast_beacons();
int tsd_ack_registration();

#endif
