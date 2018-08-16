#include "wifi_ops.h"
#include "log.h"
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <unistd.h>

/* interface handle will be returned by handle argument*/
int wifi_open(char *name, nic_handle_t **ret_handle) {

    wifi_handle_t *handle = malloc(sizeof(wifi_handle_t));

    //open rawsocket for nic "name"
    handle->sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (handle->sockfd == -1) {
        LOG_INFO("opening socket for %s is failed\n", name);
        return -1;
    }

    //set reuseaddr option for socket
    int opt = 1;
    if (setsockopt(handle->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_INFO("set reuse addr failed for socket of %s\n", name);
        goto err_ret;
    }

    //bind socket with nic
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(if_idx));
    strcpy(if_idx.ifr_name, name);
    if (ioctl(handle->sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        LOG_INFO("unable to get index of nic %s\n", name);
        goto err_ret;
    }

    struct sockaddr_ll sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = AF_PACKET;
    sock_addr.sll_protocol = htons(ETH_P_ALL);
    sock_addr.sll_ifindex = if_idx.ifr_ifindex;

    if (bind(handle->sockfd, (const struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        LOG_INFO("unable to bind %s with its' socket\n", name);
        goto err_ret;
    }

    //set index of handle
    handle->if_index = if_idx.ifr_ifindex;

    //get mac of nic
    struct ifreq if_mac;
    memset(&if_mac, 0, sizeof(if_mac));
    strcpy(if_mac.ifr_name, name);
    if (ioctl(handle->sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        LOG_INFO("unable to get mac addr of %s\n", name);
        goto err_ret;
    }

    //set mac of handle
    memcpy(handle->if_mac, (char*)&if_mac.ifr_hwaddr.sa_data, MAC_ADDR_LEN);

    strcpy(handle->name, name);

    *ret_handle = (nic_handle_t*)handle;
    return 0;

err_ret:
    close(handle->sockfd);
    return -1;
}

int wifi_close(nic_handle_t *handle) {
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    close(wifi_handle->sockfd);
    return 0;
}

int wifi_send(nic_handle_t *handle, packet_t *pkt) {
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    struct sockaddr_ll sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = AF_PACKET;
    sock_addr.sll_ifindex = wifi_handle->if_index;
    sock_addr.sll_halen = MAC_ADDR_LEN;
    
    struct ether_header *eth = (struct ether_header*)pkt->buf;
    memcpy(sock_addr.sll_addr, eth->ether_dhost, MAC_ADDR_LEN);
    
    if (sendto(wifi_handle->sockfd, pkt->buf, pkt->byte_len, 0, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        LOG_INFO("%s send packet failed\n", wifi_handle->name);
        return -1;
    }
    return 0;
}

int wifi_receive(nic_handle_t *handle, packet_t *pkt) {
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    int numbytes = recvfrom(wifi_handle->sockfd, pkt->buf, pkt->size, 0, NULL, NULL);
    if (numbytes <= 0) {
        LOG_INFO("%s is unable to receive a valid packet\n", wifi_handle->name);
        return -1;
    }
    pkt->byte_len = numbytes;
    return 0;
}

int wifi_get_info(nic_handle_t *handle, nic_info_t *info) {
    return 0;
}
