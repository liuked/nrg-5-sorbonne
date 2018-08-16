#include "wifi_ops.h"
#include "packet.h"
#include <stdio.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

static char dst_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static packet_t* prepare_packet(char *src_mac) {
    packet_t *pkt = alloc_packet(1024);
    struct ether_header *eth = (struct ether_header*)pkt->buf;
    memset(pkt->buf, 0, pkt->size);
    memcpy(eth->ether_shost, src_mac, 6);
    memcpy(eth->ether_dhost, dst_mac, 6);
    eth->ether_type = htons(ETH_P_IP);
    pkt->byte_len = sizeof(struct ether_header);
    pkt->buf[pkt->byte_len++] = 0xde;
    pkt->buf[pkt->byte_len++] = 0xad;
    pkt->buf[pkt->byte_len++] = 0xbe;
    pkt->buf[pkt->byte_len++] = 0xef;
    return pkt;
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "argv[1] = interface name\n");
        exit(-1);
    }

    char *ifname = argv[1];
    nic_handle_t *handle;

    if (wifi_open(ifname, &handle) < 0) {
        fprintf(stderr, "wifi open failed\n");
        exit(-1);
    }

    while (1) {
        packet_t *pkt = prepare_packet(((wifi_handle_t*)handle)->if_mac);
        //wifi_send(handle, pkt);
        free_packet(pkt);
        pkt = alloc_packet(1024);
        wifi_receive(handle, pkt);
        printf("received pkt %d bytes\n", pkt->byte_len);
        struct ether_header *eth = (struct ether_header*)pkt->buf;
#define smac(x) eth->ether_shost[x]
#define dmac(x) eth->ether_dhost[x]
        printf("from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X\n",
                smac(0), smac(1), smac(2), smac(3), smac(4), smac(5),
                dmac(0), dmac(1), dmac(2), dmac(3), dmac(4), dmac(5));
#undef smac
#undef dmac
        free_packet(pkt);
    }

    return 0;
}
