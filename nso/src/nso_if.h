#ifndef __NSO_IF_H__
#define __NSO_IF_H__

#include "nic_ops.h"

#define MAX_NSO_IFNAME_SZ 128

typedef struct nso_if_s {
    nic_handle_t *handle;
    nic_ops_t *ops;
    char if_name[MAX_NSO_IFNAME_SZ];
    int if_index;
}nso_if_t;


#endif
