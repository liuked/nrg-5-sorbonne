#include "include/ieee802154_ops.h"
#include "log.h"
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

/* interface handle will be returned by handle argument*/
int ieee802154_open(char *name, nic_handle_t **ret_handle) {

    ieee802154_handle_t *handle = malloc(sizeof(ieee802154_handle_t));

    //open rawsocket for nic "name"
    handle->sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IEEE802154));
    if (handle->sockfd == -1) {
        LOG_DEBUG("opening socket for %s is failed\n", name);
        goto err_ret_noclose;
    }

    //set reuseaddr option for socket
    int opt = 1;
    if (setsockopt(handle->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_DEBUG("set reuse addr failed for socket of %s\n", name);
        goto err_ret;
    }

    //bind socket with nic
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(if_idx));
    strcpy(if_idx.ifr_name, name);
    if (ioctl(handle->sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        LOG_DEBUG("unable to get index of nic %s\n", name);
        goto err_ret;
    }

    struct sockaddr_ll sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = AF_PACKET;
    sock_addr.sll_protocol = htons(ETH_P_IEEE802154);
    sock_addr.sll_ifindex = if_idx.ifr_ifindex;

    if (bind(handle->sockfd, (const struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        LOG_DEBUG("unable to bind %s with its' socket\n", name);
        goto err_ret;
    }

    //set index of handle
    handle->if_index = if_idx.ifr_ifindex;

    //get mac of nic
    struct ifreq if_mac;
    memset(&if_mac, 0, sizeof(if_mac));
    strcpy(if_mac.ifr_name, name);
    if (ioctl(handle->sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        LOG_DEBUG("unable to get mac addr of %s\n", name);
        goto err_ret;
    }

    //set mac of handle
    memcpy(handle->if_mac, (char*)&if_mac.ifr_hwaddr.sa_data, IEEE802154_LONG_ADDRESS_LEN);

    //get mtu
    if (ioctl(handle->sockfd, SIOCGIFMTU, &if_mac) < 0) {
        LOG_DEBUG("unable to get mtu if %s\n", name);
        goto err_ret;
    }
    handle->if_mtu = if_mac.ifr_mtu;

    *ret_handle = (nic_handle_t*)handle;
    return 0;


err_ret:
    close(handle->sockfd);
err_ret_noclose:
    free(handle);
    return -1;
}

int ieee802154_close(nic_handle_t *handle) {
    ieee802154_handle_t *ieee802154_handle = (ieee802154_handle_t*)handle;
    close(ieee802154_handle->sockfd);
    free(ieee802154_handle);
    return 0;
}

int ieee802154_send(nic_handle_t *handle, packet_t *pkt, l2addr_t *dst) {
    ieee802154_handle_t *ieee802154_handle = (ieee802154_handle_t*)handle;
    struct sockaddr_ll sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = AF_PACKET;
    sock_addr.sll_ifindex = ieee802154_handle->if_index;
    sock_addr.sll_halen = IEEE802154_LONG_ADDRESS_LEN;

    if (pkt->data - pkt->buf < NGR5_IEEE802154_HDR_LEN) {
        LOG_DEBUG("headroom is not enough\n");
        return -1;
    }
    
    //add ieee802154 header
    uint8_t flags = IEEE802154_FCF_ACK_REQ | IEEE802154_FCF_TYPE_DATA;

    uint8_t seq = 0x69;
    uint8_t *hdr = (uint8_t*)pkt->data - NGR5_IEEE802154_HDR_LEN;
    size_t hdrlen = ieee802154_set_frame_hdr(hdr, (uint8_t*)ieee802154_handle->if_mac, (size_t)IEEE802154_LONG_ADDRESS_LEN, (uint8_t*)dst->addr, (size_t)IEEE802154_LONG_ADDRESS_LEN, flags, seq);

    pkt->data = hdr;
    pkt->byte_len += hdrlen;

    memcpy(sock_addr.sll_addr, dst->addr, IEEE802154_LONG_ADDRESS_LEN);
    
    if (sendto(ieee802154_handle->sockfd, pkt->data, pkt->byte_len, 0, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        LOG_DEBUG("send packet failed\n");
        return -1;
    }
    return 0;
}

int ieee802154_broadcast(nic_handle_t *handle, packet_t *pkt) {
    l2addr_t *addr = alloc_l2addr(IEEE802154_LONG_ADDRESS_LEN, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
    int ret = ieee802154_send(handle, pkt, addr);
    free_l2addr(addr);
    return ret;
}

int ieee802154_receive(nic_handle_t *handle, packet_t *pkt, l2addr_t **src, l2addr_t **dst) {
    ieee802154_handle_t *ieee802154_handle = (ieee802154_handle_t*)handle;
    if (pkt->data - pkt->buf < NGR5_IEEE802154_HDR_LEN) {
        LOG_DEBUG("headroom of packet is not enough!\n");
        return -1;
    }
    pkt->data -= NGR5_IEEE802154_HDR_LEN;
    int numbytes = recvfrom(ieee802154_handle->sockfd, pkt->data, pkt->size + NGR5_IEEE802154_HDR_LEN, 0, NULL, NULL);
    if (numbytes <= 0) {
        LOG_DEBUG("unable to receive a valid packet\n");
        goto err_ret;
    }

    //extract ieee802.15.4 header
    uint8_t *hdr = (uint8_t*)pkt->data;
    if(ieee802154_get_frame_hdr_len(hdr) != NGR5_IEEE802154_HDR_LEN) {
        memset(pkt->data, 0, numbytes);
        goto err_ret;
    }

    uint8_t source[IEEE802154_LONG_ADDRESS_LEN], dest[IEEE802154_LONG_ADDRESS_LEN];

    if (ieee802154_get_src(hdr, source) != 8 ){
        LOG_DEBUG("frame header error, cannot retrieve src address!\n");
        return -1;
    }
    if (ieee802154_get_src(hdr, dest) != 8 ){
        LOG_DEBUG("frame header error, cannot retrieve dst address!\n");
        return -1;
    }


    *src = alloc_l2addr(IEEE802154_LONG_ADDRESS_LEN, source);
    *dst = alloc_l2addr(IEEE802154_LONG_ADDRESS_LEN, dest);
    pkt->data += NGR5_IEEE802154_HDR_LEN;
    pkt->byte_len = numbytes - NGR5_IEEE802154_HDR_LEN;
    pkt->tail = pkt->data + pkt->byte_len;
    return 0;

err_ret:
    pkt->data += sizeof(struct ethhdr);
    return -1;
}

int ieee802154_get_info(nic_handle_t *handle, nic_info_t *info) {
    ieee802154_handle_t *ieee802154_hdl = (ieee802154_handle_t*)handle;
    info->mtu = ieee802154_hdl->if_mtu;
    return 0;
}
