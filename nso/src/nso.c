#include "nso.h"
#include "nso_tsd.h"
#include "nso_son.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "util.h"

nso_layer_t nso_layer;

/*
 * @config_file format: 
 * <dev_id>
 * <number of ifaces>
 * <iface1> <type>
 * <iface2> <type>
 * ...
 */
static int __nso_layer_init(char *config_file) {
    //read configuration file to initialize nso_layer structure
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        LOG_DEBUG("configuration file error!\n");
        return -1;
    }

    uint64_t dev_id;
    int if_nb, if_type;
    char if_name[MAX_NSO_IFNAME_SZ];

    fscanf(fp, "%lld%d", &dev_id, &if_nb);
    if (if_nb <= 0 || if_nb > NSO_MAX_SUPPORTED_IFACES) {
        LOG_DEBUG("interface number error\n");
        goto err_ret;
    }

    int if_id;
    for (if_id = 0; if_id < if_nb; if_id++) {
        fscanf(fp, "%s%d", if_name, &if_type);
        if (if_type < IFACE_TYPE_MIN ||
                if_type >= IFACE_TYPE_MAX) {
            LOG_DEBUG("interface type error!\n");
            goto err_free_ifaces;
        }
        nso_layer.ifaces[if_id] = alloc_nso_if(if_name, if_id, if_type);
        if (!nso_layer.ifaces[if_id]) {
            goto err_free_ifaces;
        }
    }

    nso_layer.ifaces_nb = if_nb;

    nso_layer.dev_id = alloc_device_id((uint8_t*)&dev_id);

    if (!nso_layer.dev_id) {
        goto err_free_ifaces;
    }

    nso_layer.gw_id = alloc_device_id(NULL);

    if (!nso_layer.gw_id) {
        goto err_free_devid;
    }

    nso_layer.dev_state = NRG5_UNREG;
    pthread_mutex_init(&nso_layer.state_lock, NULL);
    pthread_cond_init(&nso_layer.state_signal, NULL);

    nso_layer.arpt = arp_table_create();
    if (!nso_layer.arpt) {
        goto err_destroy_lock;
    }

    nso_layer.son_fwdt = fwd_table_create();
    if (!nso_layer.son_fwdt) {
        goto err_free_arpt;
    }

    nso_layer.local_fwdt = fwd_table_create();
    if (!nso_layer.local_fwdt) {
        goto err_free_son;
    }

    nso_layer.nbrt = nbr_table_create();
    if (!nso_layer.nbrt) {
        goto err_free_local;
    }

    //open all interfaces
    int i;
    nic_info_t info;
    nso_layer.mtu = -1;
    for (i = 0; i < nso_layer.ifaces_nb; i++) {
        if (nso_if_open(nso_layer.ifaces[i])) {
            goto err_close_if;
        }
        //get mtu here
        nso_if_get_info(nso_layer.ifaces[i], &info);
        nso_layer.mtu = nso_layer.mtu > info.mtu ? nso_layer.mtu: info.mtu;
    }

    nso_layer.timeout_ms = NSO_DEFAULT_TIMEOUT_MS;
    nso_layer.aging_period_ms = NSO_DEFAULT_AGING_PERIOD_MS;
    nso_layer.sr_period_ms = NSO_DEFAULT_SON_REPORT_PERIOD_MS;

    //TODO: implement battery reader to get real battery information from device
    nso_layer.battery = 100;

    tsd_init();

    //init success
    fclose(fp);
    return 0;

err_close_if:
    while (i--) {
        nso_if_close(nso_layer.ifaces[i]);
    }
err_free_nbrt:
    nbr_table_free(nso_layer.nbrt);
err_free_local:
    fwd_table_free(nso_layer.local_fwdt);
err_free_son:
    fwd_table_free(nso_layer.son_fwdt);
err_free_arpt:
    arp_table_free(nso_layer.arpt);
err_destroy_lock:
    pthread_mutex_destroy(&nso_layer.state_lock);
    pthread_cond_destroy(&nso_layer.state_signal);
err_free_devid:
    free_device_id(nso_layer.dev_id);
err_free_ifaces:
    while(if_id--) {
        free_nso_if(nso_layer.ifaces[if_id]);
    }
err_ret:
    fclose(fp);
    return -1;
}

static void* __rx_thread_main(void *arg) {
    return NULL;
}

static void* __tx_thread_main(void *arg) {
    //register
    struct timespec timeout;
    pthread_mutex_lock(&nso_layer.state_lock);
restart_registration:
    while (nso_layer.dev_state == NRG5_UNREG) {
        pthread_mutex_unlock(&nso_layer.state_lock);
        //broadcast
        tsd_broadcast_beacons();
        //wait for registration success
        pthread_mutex_lock(&nso_layer.state_lock);
        make_timeout(&timeout, nso_layer.timeout_ms);
        pthread_cond_timedwait(&nso_layer.state_signal, &nso_layer.state_lock, &timeout);
    }
    pthread_mutex_unlock(&nso_layer.state_lock);

    LOG_INFO("device is registered into NRG-5 database!\n");

    //send first topo report
    son_topo_report();

    //wait for first routes update
    pthread_mutex_lock(&nso_layer.state_lock);
    make_timeout(&timeout, nso_layer.timeout_ms);
    pthread_cond_timedwait(&nso_layer.state_signal, &nso_layer.state_lock, &timeout);
    //once the first routes update is received, rx thread will update son table and set dev_state to NRG5_CONNECTED
    if (nso_layer.dev_state != NRG5_CONNECTED) {
        LOG_DEBUG("timeout for waiting first vSON route update!\n");
        goto restart_registration;
    }
    pthread_mutex_unlock(&nso_layer.state_lock);

    //send periodical report & neighbor advertisement
    pthread_mutex_lock(&nso_layer.state_lock);
    while (nso_layer.dev_state == NRG5_CONNECTED) {
        pthread_mutex_unlock(&nso_layer.state_lock);
        //send topo report
        son_topo_report();
        son_nbr_advertise();
        //sleep for a period of time
        //TODO: sleep
        usleep(nso_layer.sr_period_ms * 1000);
        pthread_mutex_lock(&nso_layer.state_lock);
    }
    pthread_mutex_unlock(&nso_layer.state_lock);

    //once device is unconnectted
    //TODO: restart registration to support mobility
    //e.g. reset all states and tables

    return NULL;
}

static void* __aging_thread_main(void *arg) {
    return NULL;
}

int nso_layer_run(char *config_file) {
    int ret;
    ret = __nso_layer_init(config_file);
    if (ret) {
        LOG_DEBUG("nso layer init failed!\n");
        return -1;
    }
    pthread_create(&nso_layer.rx_pid, NULL, __rx_thread_main, NULL);
    pthread_create(&nso_layer.tx_pid, NULL, __tx_thread_main, NULL);
    pthread_create(&nso_layer.aging_pid, NULL, __aging_thread_main, NULL);
    LOG_INFO("nso layer running!\n");
    return 0;
}

int nso_layer_stop() {
    //cancel threads
    //free data structure
    return 0;
}

int nso_layer_fwd(packet_t *pkt) {
    struct nsohdr *nso_hdr = (struct nsohdr*)pkt->data;
    device_id_t *dst = alloc_device_id(nso_hdr->dst_devid);
    device_id_t *nxt = NULL;
    nso_if_t *iface = NULL;

    fwd_table_lock(nso_layer.son_fwdt);
    fwd_entry_t *fwd_e = fwd_table_lookup_unsafe(nso_layer.son_fwdt, dst);
    if (fwd_e) {
        nxt = alloc_device_id((uint8_t*)fwd_e->nxthop);
        iface = fwd_e->interface;
    }
    fwd_table_unlock(nso_layer.son_fwdt);

    if (!nxt) {
        fwd_table_lock(nso_layer.local_fwdt);
        fwd_e = fwd_table_lookup_unsafe(nso_layer.local_fwdt, dst);
        if (fwd_e) {
            nxt = alloc_device_id((uint8_t*)fwd_e->nxthop);
            iface = fwd_e->interface;
        }
        fwd_table_unlock(nso_layer.local_fwdt);
    }

    int ret = 0;
    if (!nxt) {
        LOG_DEBUG("no avaliable forwarding entry!\n");
        ret = -1;
        goto out;
    }
    arp_table_lock(nso_layer.arpt);
    arp_entry_t *arp_e = arp_table_lookup_from_dev_id_unsafe(nso_layer.arpt, nxt);
    if (!arp_e) {
        LOG_DEBUG("no avaliable arp entry!\n");
        ret = -1;
        goto out;
    }
    l2addr_t *addr = alloc_l2addr(arp_e->l2addr->size, arp_e->l2addr->addr);
    arp_table_unlock(nso_layer.arpt);
    nso_if_send(iface, pkt, addr);
out:
    free_device_id(dst);
    free_device_id(nxt);
    free_l2addr(addr);
    return ret;
}
