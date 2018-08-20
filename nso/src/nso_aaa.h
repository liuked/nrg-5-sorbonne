#ifndef __NSO_AAA_H__
#define __NSO_AAA_H__

#include <stdint.h>

//actual data size in bytes is returned
int nso_get_credentials(uint8_t *buf, int size);


#endif
