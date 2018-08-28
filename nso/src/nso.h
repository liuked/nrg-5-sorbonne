#ifndef __NSO_H__
#define __NSO_H__

#include "arp.h"
#include "fwding.h"
#include "nso_if.h"
#include "neighbor.h"
#include "nso_common.h"
#include <pthread.h>

#define NSO_MAX_SUPPORTED_IFACES 10


typedef enum {
    NRG5_UNREG = 0,
    NRG5_REG = 1,
    NRG5_CONNECTED = 2,
}device_status_e;


typedef struct {
    
    //interface list
    nso_if_t *ifaces[NSO_MAX_SUPPORTED_IFACES];
    int ifaces_nb;

    //arp table
    arp_table_t *arpt;

    //fwding table
    fwd_table_t *son_fwdt;
    fwd_table_t *local_fwdt;

    //neighbor table
    nbr_table_t *nbrt;

    //gw device id
    device_id_t *gw_id;

    //device status
    device_status_e dev_state;

}nso_layer_t;


#endif
