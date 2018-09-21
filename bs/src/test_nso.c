#include "nso.h"
#include "api.h"
#include <string.h>

static void __app_main() {
    char buf[1024] = "hello nrg5!";
    int size = strlen(buf);
    uint16_t proto;
    int mtu = nso_get_mtu();
    while(1) {
        int send_bytes = nso_send(buf, size, NULL, 0xFF3F);
        printf("send %d bytes!\n", send_bytes);
        int recv_bytes = nso_receive(buf, 1024, NULL, NULL, &proto);
        printf("recv %d bytes!\n proto %d\n", recv_bytes, proto);
        buf[recv_bytes] = 0;
        printf("data %s\n", buf);
    }
}

int main(){
    nso_layer_run("../config/config_example");
    __app_main();
    return 0;
}
