#ifndef __NSO_PACKET_H__
#define __NSO_PACKET_H__

#include <stdint.h>
#include "nso_common.h"

#define NSO_PROTO_IP 0x0800
#define NSO_PROTO_IPv6 0x86dd
#define NSO_PROTO_CP_DEV_REG 0xF001
#define NSO_PROTO_CP_DEV_REG_REPLY 0xF002
#define NSO_PROTO_CP_TOPO_REPO 0xF003
#define NSO_PROTO_CP_RT_UPDATE 0xF004

#define NSO_PROTO_IS_CP(proto) ((proto & 0xFFF0) == 0xF000)

struct nsohdr {
    uint8_t src_devid[DEV_ID_WIDTH];
    uint8_t dst_devid[DEV_ID_WIDTH];
    uint16_t proto;
    //length including header size
    uint16_t length;
}__attribute__((packed));

#endif
