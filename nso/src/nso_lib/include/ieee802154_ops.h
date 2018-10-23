#ifndef __IEEE802154_OPS_H__
#define __iEEE802154_OPS_H__

#include "packet.h"
#include "nic_ops.h"
#include "ieee802154.h"

#define IEEE802154_MAX_PKT_LEN 127
#define NGR5_IEEE802154_HDR_LEN 19

#ifndef ETH_P_IEEE802154
#define ETH_P_IEEE802154 0x00F6
#endif


struct ieee802154_handle_s {
    int sockfd;
    int if_index;
    char if_mac[ETH_ALEN];
    int if_mtu;
};

typedef struct ieee802154_handle_s ieee802154_handle_t;

int ieee802154_open(char *name, nic_handle_t **handle);
int ieee802154_close(nic_handle_t *handle);
int ieee802154_send(nic_handle_t *handle, packet_t *pkt, l2addr_t *dst);
int ieee802154_broadcast(nic_handle_t *handle, packet_t *pkt);
int ieee802154_receive(nic_handle_t *handle, packet_t *pkt, l2addr_t **src, l2addr_t **dst);
int ieee802154_get_info(nic_handle_t *handle, nic_info_t *info);

#endif
