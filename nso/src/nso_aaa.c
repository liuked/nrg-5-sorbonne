#include "nso_aaa.h"
#include <stdlib.h>

const uint8_t test_credentials[] = {
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
};

//as we don't have the AAA implemetation now, just use this simple code to simulate the behaviour of AAA
int aaa_get_credentials(uint8_t *in_buf, int in_size, uint8_t *out_buf, int *out_size) {
    memcpy(out_buf, test_credentials, 8);
    *out_size = 8;
    return 0;
}
