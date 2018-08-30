#ifndef __NSO_AAA_H__
#define __NSO_AAA_H__

#include <stdint.h>

int aaa_get_credentials(uint8_t *in_buf, int in_size, uint8_t *out_buf, int *out_size);


#endif
