#ifndef __WIFI_OPS_H__
#define __WIFI_OPS_H__

#include "packet.h"
#include "nic_ops_intf.h"

#define MAC_ADDR_LEN 6

struct wifi_handle_s {
    int sockfd;
    int if_index;
    char if_mac[MAC_ADDR_LEN];
    char name[256];
};

typedef struct wifi_handle_s wifi_handle_t;

int wifi_open(char *name, nic_handle_t **handle);
int wifi_close(nic_handle_t *handle);
int wifi_send(nic_handle_t *handle, packet_t *pkt);
int wifi_receive(nic_handle_t *handle, packet_t *pkt);
int wifi_get_info(nic_handle_t *handle, nic_info_t *info);

#endif
