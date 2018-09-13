#include "nso_tsd.h"
#include "nso.h"
#include "nso_packet.h"
#include "packet.h"
#include "nso_aaa.h"
#include <arpa/inet.h>

extern nso_layer_t nso_layer;
static nso_tsd_t nso_tsd;

/*
 * message format: [NSO hdr][type 1B][credential XB]
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
    if (aaa_get_credentials(nso_hdr->src_devid, DEV_ID_WIDTH, 
                aaa_data, &aaa_len) < 0) {
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
    ret = nso_if_broadcast(iface, pkt);
    //l2addr_t *dst = alloc_l2addr(6, NULL);
    //dst->addr[0] = 0x50;
    //dst->addr[1] = 0x3e;
    //dst->addr[2] = 0xaa;
    //dst->addr[3] = 0x49;
    //dst->addr[4] = 0x04;
    //dst->addr[5] = 0x90;
    //ret = nso_if_send(iface, pkt, dst);

    free_packet(pkt);
out:
    nso_tsd.if_index = (nso_tsd.if_index + 1) % nsol->ifaces_nb;
    return ret;
}


/*
 * msg format:
 * [nso hdr]
 * [type 1B]
 * [type related data xB]
 * */

static void __fwd_beacon(nso_layer_t *nsol, packet_t *pkt, l2addr_t *src, nso_if_t *iface) {
    int ret = 0;
    pthread_mutex_lock(&nsol->state_lock);
    ret = nsol->dev_state == NRG5_CONNECTED;
    pthread_mutex_unlock(&nsol->state_lock);
    if (!ret) {
        LOG_DEBUG("drop received beacon\n");
        return;
    }

    struct nsohdr *hdr = (struct nsohdr*)pkt->data;
    device_id_t *dst_id = alloc_device_id(hdr->dst_devid);
    //add reverse temporary route for beacon
    if (is_broadcast_dev_id(dst_id)) {
        memcpy(hdr->dst_devid, (uint8_t*)nsol->gw_id, DEV_ID_WIDTH);
        device_id_t *src_id = alloc_device_id(hdr->src_devid);
        l2addr_t *src_mac = alloc_l2addr(src->size, src->addr);
        arp_table_lock(nsol->arpt);
        arp_entry_t *arp_e = arp_table_lookup_from_dev_id_unsafe(nsol->arpt, src_id);
        if (arp_e)
            arp_e->status = ARP_ACTIVE;
        arp_table_unlock(nsol->arpt);
        if (!arp_e) {
            arp_e = alloc_arp_entry(src_id, src_mac);
            arp_table_add(nsol->arpt, arp_e);
        } else {
            free_device_id(src_id);
            free_l2addr(src_mac);
        }
        src_id = alloc_device_id(hdr->src_devid);
        fwd_table_lock(nsol->local_fwdt);
        fwd_entry_t *fwd_e = fwd_table_lookup_unsafe(nsol->local_fwdt, src_id);
        if (fwd_e)
            fwd_e->status = FWD_ACTIVE;
        fwd_table_unlock(nsol->local_fwdt);

        if (!fwd_e) {
            fwd_e = alloc_fwd_entry(src_id, NULL, iface);
            fwd_table_add(nsol->local_fwdt, fwd_e);
        } else {
            free_device_id(src_id);
        }
    } else {
        device_id_t *nxt_id;
        arp_table_lock(nsol->arpt);
        arp_entry_t *arp_e = arp_table_lookup_from_l2addr_unsafe(nsol->arpt, src);
        if (!arp_e) {
            LOG_DEBUG("don't have an arp entry for downstream device!\n");
            exit(-1);
        }
        nxt_id = alloc_device_id((uint8_t*)arp_e->dev_id);
        arp_table_unlock(nsol->arpt);
        device_id_t *src_id = alloc_device_id(hdr->src_devid);
        fwd_entry_t *fwd_e = alloc_fwd_entry(src_id, nxt_id, iface);
        fwd_table_add(nsol->local_fwdt, fwd_e);
    }
    free_device_id(dst_id);
    nso_layer_fwd(pkt);
}


#define NRG5_REG_SUCCESS 0
/*
 * msg format:
 * [answer 1B]: 0 = success, 1 = fail
 * */
static void __process_reg_reply(nso_layer_t *nsol, packet_t *pkt, l2addr_t *src, nso_if_t *iface) {
    struct nsohdr *hdr = (struct nsohdr*)pkt->data;
    device_id_t *dst_id = alloc_device_id((uint8_t*)hdr->dst_devid);
    if (device_id_equal(dst_id, nsol->dev_id)) {
        //is for me 
        pthread_mutex_lock(&nsol->state_lock);
        uint8_t ans = *(pkt->data + sizeof(struct nsohdr) + 1);
        if (nsol->dev_state == NRG5_UNREG && ans == NRG5_REG_SUCCESS) {
            device_id_t *nxt_id;
            arp_table_lock(nsol->arpt);
            arp_entry_t *arp_e = arp_table_lookup_from_l2addr_unsafe(nsol->arpt, src);
            if (!arp_e) {
                LOG_DEBUG("don't have an arp entry for downstream device!\n");
                exit(-1);
            }
            nxt_id = alloc_device_id((uint8_t*)arp_e->dev_id);
            arp_table_unlock(nsol->arpt);
            device_id_t *src_id = alloc_device_id(hdr->src_devid);
            fwd_entry_t *fwd_e = alloc_fwd_entry(src_id, nxt_id, iface);
            fwd_table_add(nsol->local_fwdt, fwd_e);

            nsol->gw_id = alloc_device_id((uint8_t*)src_id);
            nsol->dev_state = NRG5_REG;
            pthread_cond_broadcast(&nsol->state_signal);
        } else {
            LOG_DEBUG("drop received beacon reply\n");
        }
        pthread_mutex_unlock(&nsol->state_lock);
    } else {
        //forward
        pthread_mutex_lock(&nsol->state_lock);
        int ret = nsol->dev_state == NRG5_CONNECTED;
        pthread_mutex_unlock(&nsol->state_lock);
        if (!ret) {
            LOG_DEBUG("drop received beacon reply\n");
        } else {
            nso_layer_fwd(pkt);
        }
    }
    free_device_id(dst_id);
}

int tsd_process_rx(packet_t *pkt, l2addr_t *src, l2addr_t *dst, nso_if_t *iface) {
    nso_layer_t *nsol = &nso_layer;
    uint8_t type = *(pkt->data + sizeof(struct nsohdr));
    switch(type) {
        case NSO_TSD_MSG_REG:
            __fwd_beacon(nsol, pkt, src, iface);
            break;
        case NSO_TSD_MSG_REG_REPLY:
            __process_reg_reply(nsol, pkt, src, iface);
            break;
        default:
            LOG_DEBUG("unknown tsd message\n");
            return -1;
    }
    return 0;
}
