#include "nso_aaa.h"
#include <stdlib.h>


uint8_t credentials[] = {
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
};

int nso_get_credentials(uint8_t *buf, int size) {
    int data_size = sizeof(credentials);
    memcpy(buf, credentials, data_size);
    return data_size;
}
