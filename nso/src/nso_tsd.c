#include "nso_tsd.h"
#include "nso.h"
#include "nso_packet.h"
#include "packet.h"
#include "nso_aaa.h"
#include <arpa/inet.h>

static nso_tsd_t nso_tsd;

/*
 * message format: [NSO hdr][type 1B][credential xB]
 */
static packet_t* __get_nso_beacon(nso_layer_t *nsol) {
    packet_t *pkt =  alloc_packet(nsol->mtu);
    //fill in data
    struct nsohdr *nso_hdr = (struct nsohdr*)pkt->data;

    memcpy(nso_hdr->src_devid, (uint8_t*)nsol->dev_id, DEV_ID_WIDTH);

    device_id_t *bc_id = alloc_bc_device_id();
    memcpy(nso_hdr->dst_devid, (uint8_t*)bc_id, DEV_ID_WIDTH);
    free_device_id(bc_id);

    nso_hdr->proto = htons(NSO_PROTO_CP_VTSD);
    nso_hdr->ver = 0;

    uint8_t *type = pkt->data + sizeof(struct nsohdr);
    uint8_t *aaa_data = type + 1;

    *type = NSO_TSD_MSG_REG;

    int aaa_len;
    if (aaa_get_credentials(nso_hdr->src_devid, DEV_ID_WIDTH, aaa_data, &aaa_len) < 0) {
        LOG_DEBUG("unable to get credential!\n");
        free_packet(pkt);
        return NULL;
    }
    nso_hdr->length = aaa_len + sizeof(struct nsohdr) + sizeof(*type);
    
    //maintain pointers
    packet_inc_len(pkt, nso_hdr->length);
        
    nso_hdr->len_ver = htons(nso_hdr->len_ver);
    return pkt;
}

int tsd_init() {
    nso_tsd.if_index = 0;
    return 0;
}

int tsd_broadcast_beacons() {
    nso_layer_t *nsol = &nso_layer;
    packet_t *pkt = __get_nso_beacon(nsol);
    int ret = 0;
    if (!pkt) {
        ret = -1;
        goto out;
    }
    nso_if_t *iface = nsol->ifaces[nso_tsd.if_index];
    //ret = nso_if_broadcast(iface, pkt);
    l2addr_t *dst = alloc_l2addr(6, NULL);
    dst->addr[0] = 0x50;
    dst->addr[1] = 0x3e;
    dst->addr[2] = 0xaa;
    dst->addr[3] = 0x49;
    dst->addr[4] = 0x04;
    dst->addr[5] = 0x90;
    ret = nso_if_send(iface, pkt, dst);

    free_packet(pkt);
out:
    nso_tsd.if_index = (nso_tsd.if_index + 1) % nsol->ifaces_nb;
    return ret;
}

int tsd_ack_registration() {
    nso_layer_t *nsol = &nso_layer;
    pthread_mutex_lock(&nsol->state_lock);
    nsol->dev_state = NRG5_REG;
    pthread_mutex_unlock(&nsol->state_lock);
    pthread_cond_broadcast(&nsol->state_signal);
    return 0;
}
