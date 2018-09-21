#include "nso.h"
#include "api.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#define USAGE_PER_SEC 6.6916087962962962962962962962963e-4
#define REPORT_RATE_S 3
#define APP_PROTO 0x0800

typedef struct {
    nso_addr_t dev_id;
    uint8_t stop_charge;
    double usage;
}power_meter_t;

power_meter_t power_meter;

static int fill_data_buf(uint8_t *buf, nso_addr_t *dev_id, double usage) {
    memcpy(buf, (uint8_t*)dev_id, NSO_ADDR_LEN);
    memcpy(buf + NSO_ADDR_LEN, (uint8_t*)&usage, sizeof(usage));
    return NSO_ADDR_LEN + sizeof(usage);
}

static void init_power_meter(power_meter_t *pw) {
    nso_get_device_id(&pw->dev_id);
    pw->stop_charge = 0;
    pw->usage = 0;
}

static void* __app_tx(void *arg){
    char buf[1024];
    int size;
    uint16_t proto;
    while(1) {
        if (!power_meter.stop_charge) {
            size = fill_data_buf(buf, &power_meter.dev_id, power_meter.usage);
            int send_bytes = nso_send(buf, size, NULL, APP_PROTO);
            printf("send %d bytes! [%llx %lf]\n", send_bytes, *(uint64_t*)&power_meter.dev_id, power_meter.usage);
            sleep(REPORT_RATE_S);
            power_meter.usage += USAGE_PER_SEC * REPORT_RATE_S;
        }
    }
    return NULL;
}

#define CMD_STOP_CHARGE 0
#define CMD_START_CHARGE 1

static void process_cmd(uint8_t *buf, int size, power_meter_t *pw) {
    switch(*buf) {
        case CMD_STOP_CHARGE:
            pw->stop_charge = 1;
            printf("stop charge!\n");
            break;
        case CMD_START_CHARGE:
            pw->stop_charge = 0;
            printf("start charge!\n");
        default:
            printf("unknown cmd!\n");
            break;
    }
}

static void* __app_rx(void *arg){
    char rbuf[1024];
    uint16_t proto;
    while(1) {
        int recv_bytes = nso_receive(rbuf, 1024, NULL, NULL, &proto);
        printf("recv %d bytes!\n proto %d\n", recv_bytes, proto);
        process_cmd(rbuf, recv_bytes, &power_meter);
    }
    return NULL;
}

static void __app_main() {
    pthread_t tx, rx;
    init_power_meter(&power_meter);
    pthread_create(&tx, NULL, __app_tx, NULL);
    __app_rx(NULL);
}

int main(){
    nso_layer_run("../config/config_example");
    __app_main();
    return 0;
}
