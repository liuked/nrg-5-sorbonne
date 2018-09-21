#include "nso.h"
#include "api.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static void* __app_tx(void *arg){
    char buf[1024] = "hello nrg5!";
    int size;
    int seq = 0;
    uint16_t proto;
    int mtu = nso_get_mtu();
    while(1) {
        sprintf(buf, "hello %d!", seq++);
        size = strlen(buf);
        int send_bytes = nso_send(buf, size, NULL, 0x0800);
        printf("send %d bytes! [%s]\n", send_bytes, buf);
        sleep(1);
    }
    return NULL;
}

static void* __app_rx(void *arg){
    char rbuf[1024];
    uint16_t proto;
    while(1) {
        int recv_bytes = nso_receive(rbuf, 1024, NULL, NULL, &proto);
        printf("recv %d bytes!\n proto %d\n", recv_bytes, proto);
        rbuf[recv_bytes] = 0;
        printf("data %s\n", rbuf);
    }
    return NULL;
}

static void __app_main() {
    pthread_t tx, rx;
    nso_addr_t dev_id;
    nso_get_device_id(&dev_id);
    printf("device_id 0x%llx\n", *(uint64_t*)&dev_id);
    pthread_create(&tx, NULL, __app_tx, NULL);
    __app_rx(NULL);
}

int main(int argc, char **argv){
    if (argc != 2) {
        printf("pass config file as argv[1]!\n");
        return -1;
    }
    nso_layer_run(argv[1]);
    __app_main();
    return 0;
}
