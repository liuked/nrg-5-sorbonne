#include "nso_son.h"
#include "neighbor.h"
#include "nso.h"
#include "fwding.h"
#include "arp.h"

int son_init() {
    return 0;
}

/*
 * msg format:
 * [nso hdr]
 * [type 1B]
 * [topo data]
 *
 * topo data format:
 * <battery value 1B>
 * <number of interfaces 1B>
 * <iface index 1B> <iface quality 1B>
 * ...
 * <number of neighbors 2B>
 * <neighbor device id 8B> <neighbor link quality 1B> <iface index 1B>
 * ...
 * */

static packet_t* __get_topo_report_pkt(nso_layer_t *nsol) {
    packet_t *pkt =  alloc_packet(nsol->mtu);
    //fill in data
    struct nsohdr *nso_hdr = (struct nsohdr*)pkt->data;

    memcpy(nso_hdr->src_devid, (uint8_t*)nsol->dev_id, DEV_ID_WIDTH);

    //to gateway
    memcpy(nso_hdr->dst_devid, (uint8_t*)nsol->gw_id, DEV_ID_WIDTH);

    nso_hdr->proto = htons(NSO_PROTO_CP_VSON);
    nso_hdr->ver = 0;

    uint8_t *type = pkt->data + sizeof(struct nsohdr);
    uint8_t *data = type + 1;

    *type = NSO_SON_MSG_TOPO;
    
    //preprare the data 
    *data = nsol->battery;
    data += 1;
    *data = (uint8_t)nsol->ifaces_nb;
    data += 1;
    int ifid;
    for (ifid = 0; ifid < nsol->ifaces_nb; ifid++) {
        nso_if_t *iface = nsol->ifaces[ifid];
        *data = (uint8_t)ifid;
        //TODO: get interface quality from device, instead of using static value
        *(data + 1) = 0;
        data += 2;
    }

    nbr_table_lock(nsol->nbrt);

    *(uint16_t*)data = htons(nbr_table_size(nsol->nbrt));
    data += 2;

    nbr_entry_t *entry;
    nbr_table_iterate(entry, nsol->nbrt) {
        memcpy(data, (uint8_t*)entry->dev_id, DEV_ID_WIDTH);
        *(data + DEV_ID_WIDTH) = (uint8_t)entry->metric->w;
        *(data + DEV_ID_WIDTH + 1) = (uint8_t)entry->iface->if_index;
        data += DEV_ID_WIDTH + 2;
    }

    nbr_table_unlock(nsol->nbrt);

    nso_hdr->length = data - type + sizeof(struct nsohdr);

    //maintain pointers
    packet_inc_len(pkt, nso_hdr->length);
    nso_hdr->len_ver = htons(nso_hdr->len_ver);
    return pkt;
}

int son_topo_report() {
    nso_layer_t *nsol = &nso_layer;
    packet_t *pkt = __get_topo_report_pkt(nsol);
    if (!pkt) {
        return -1;
    }
    nso_layer_fwd(pkt);
    free_packet(pkt);
    return 0;
}

static packet_t* __get_nbr_adv_pkt(nso_layer_t *nsol) {
    packet_t *pkt =  alloc_packet(nsol->mtu);
    //fill in data
    struct nsohdr *nso_hdr = (struct nsohdr*)pkt->data;

    memcpy(nso_hdr->src_devid, (uint8_t*)nsol->dev_id, DEV_ID_WIDTH);

    device_id_t *bc_id = alloc_bc_device_id();
    memcpy(nso_hdr->dst_devid, (uint8_t*)bc_id, DEV_ID_WIDTH);
    free_device_id(bc_id);

    nso_hdr->proto = htons(NSO_PROTO_CP_VSON);
    nso_hdr->ver = 0;

    uint8_t *type = pkt->data + sizeof(struct nsohdr);

    *type = NSO_SON_MSG_NBR;

    nso_hdr->length = sizeof(struct nsohdr) + sizeof(*type);
    
    //maintain pointers
    packet_inc_len(pkt, nso_hdr->length);
    nso_hdr->len_ver = htons(nso_hdr->len_ver);
    return pkt;
}

int son_nbr_advertise() {
    nso_layer_t *nsol = &nso_layer;
    packet_t *pkt = __get_nbr_adv_pkt(nsol);
    if (!pkt) {
        return -1;
    }
    //advertising to all interfaces
    int i;
    for (i = 0; i < nsol->ifaces_nb; i++) {
        nso_if_t *iface = nsol->ifaces[i];
        if (nso_if_broadcast(iface, pkt) < 0) {
            LOG_DEBUG("advertise neighborship failed in iface %s\n", iface->if_name);
        }
    }
    free_packet(pkt);
    return 0;
}


/*
 * message format:
 * <1B action> <8B dst_devid> <8B nxthop_devid> <1B if_index>
 * .......
 *
 * action:
 * 0 = add
 * 1 = del
 * */
static void __route_update(nso_layer_t *nsol, uint8_t *rt_data, int size) {
    rt_rule_t *rule;
    while (size > 0) {
        rule = (rt_rule_t*)rt_data;
        device_id_t *dst_id = alloc_device_id((uint8_t*)&rule->dst_id);
        device_id_t *nh_id = alloc_device_id((uint8_t*)&rule->nh_id);
        nso_if_t *iface = nsol->ifaces[rule->if_index];
        fwd_entry_t *fwd_e = fwd_table_lookup(nsol->son_fwdt, dst_id);
        switch(rule->action) {
            case RT_ACTION_ADD:
                if (fwd_e) {
                    fwd_table_del(nsol->son_fwdt, fwd_e);
                    free_fwd_entry(fwd_e);
                }
                fwd_e = alloc_fwd_entry(dst_id, nh_id, iface);
                fwd_table_add(nsol->son_fwdt, fwd_e);
                break;
            case RT_ACTION_DEL:
                if (fwd_e) {
                    fwd_table_del(nsol->son_fwdt, fwd_e);
                    free_fwd_entry(fwd_e);
                }
                free_device_id(dst_id);
                free_device_id(nh_id);
                break;
            default:
                free_device_id(dst_id);
                free_device_id(nh_id);
                LOG_DEBUG("unknown route rule action!\n");
        }

        size -= sizeof(rt_rule_t);
        rt_data += sizeof(rt_rule_t);
    }
    assert(size == 0);
}

static void __neighbor_maintain(nsol_layer_t *nsol, struct nsohdr *hdr, l2addr_t *src, l2addr_t *dst, nso_if_t *iface) {
    device_id_t *dev_id = alloc_device_id(hdr->src_devid);
    //maintain arp table
    arp_entry_t *arp_e = arp_table_lookup_from_dev_id(nsol->arpt, dev_id);
    if (arp_e) {
        arp_e->status = ARP_ACTIVE;
    } else {
        device_id_t *dev_id_copy = alloc_device_id(hdr->src_devid);
        l2addr_t *mac = alloc_l2addr(src->size, src->addr);
        arp_e = alloc_arp_entry(dev_id_copy, mac);
        arp_table_add(nsol->arpt, arp_e);
    }
    //maintain neighbor table
    nbr_entry_t *nbr_e = nbr_table_lookup(nsol->nbrt, dev_id);
    if (nbr_e) {
        nbr_e->status = NBR_ACTIVE;
    } else {
        device_id_t *dev_id_copy = alloc_device_id(hdr->src_devid);
        metric_t *m = alloc_metric(1);
        nbr_e = alloc_nbr_entry(dev_id_copy, m, iface);
        nbr_table_add(nsol->nbrt, nbr_e);
    }
}

int son_process_rx(uint8_t *data, int size, struct nsohdr *hdr, l2addr_t *src, l2addr_t *dst, nso_if_t *iface) {
    uint8_t type = *data;
    nso_layer_t *nsol = &nso_layer;
    switch(type){
        case NSO_SON_MSG_RT:
            __route_update(nsol, data + sizeof(type), size - sizeof(type));
            break;
        case NSO_SON_MSG_NBR:
            __neighbor_maintain(nsol, hdr, src, dst, iface);
            break;
        default:
            LOG_DEBUG("unknown son message!\n");
            return -1;
    }
    return 0;
}
