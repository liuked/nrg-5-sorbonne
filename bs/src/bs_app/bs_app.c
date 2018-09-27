#include "nso.h"
#include "api.h"
#include <string.h>


static void turn_on_led() {

}

int main(int argc, char **argv){
    if (argc != 2) {
        printf("argv[1] = config file!\n");
        return -1;
    }
    nso_layer_run(argv[1]);
    while (!nso_is_connected());
    turn_on_led();
    while (1);
    return 0;
}
