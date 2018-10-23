/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author Martine Lenders <mlenders@inf.fu-berlin.de>
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "net/ieee802154.h"

const uint8_t ieee802154_addr_bcast[IEEE802154_ADDR_BCAST_LEN] = IEEE802154_ADDR_BCAST;

size_t ieee802154_set_frame_hdr(uint8_t *buf, uint8_t *src, size_t src_len,
                                uint8_t *dst, size_t dst_len,
                                uint8_t flags, uint8_t seq)
{
    int pos = 3;    /* 0-1: FCS, 2: seq */
    uint8_t type = (flags & IEEE802154_FCF_TYPE_MASK);

    buf[0] = flags;
    buf[1] = IEEE802154_FCF_VERS_V1;

    if (((src_len != 0) && (src == NULL)) ||
        ((dst_len != 0) && (dst == NULL))) {
        return 0;
    }

    /* Frame type is not beacon or ACK, but both address modes are zero */
    if ((type != IEEE802154_FCF_TYPE_BEACON) && (type != IEEE802154_FCF_TYPE_ACK) &&
        (src_len == 0) && (dst_len == 0)) {
        return 0;
    }

    /* set sequence number */
    buf[2] = seq;


    /* fill in destination address */
    switch (dst_len) {
        case 0:
            buf[1] |= IEEE802154_FCF_DST_ADDR_VOID;
            break;
        case 2:
            if (memcmp(dst, ieee802154_addr_bcast,
                       sizeof(ieee802154_addr_bcast)) == 0) {
                /* do not request ACKs for broadcast address */
                buf[0] &= ~IEEE802154_FCF_ACK_REQ;
            }
            buf[1] |= IEEE802154_FCF_DST_ADDR_SHORT;
            buf[pos++] = dst[1];
            buf[pos++] = dst[0];
            break;
        case 8:
            buf[1] |= IEEE802154_FCF_DST_ADDR_LONG;
            for (int i = 7; i >= 0; i--) {
                buf[pos++] = dst[i];
            }
            break;
        default:
            return 0;
    }

    /* fill in source address */
    switch (src_len) {
        case 0:
            buf[1] |= IEEE802154_FCF_SRC_ADDR_VOID;
            break;
        case 2:
            buf[1] |= IEEE802154_FCF_SRC_ADDR_SHORT;
            buf[pos++] = src[1];
            buf[pos++] = src[0];
            break;
        case 8:
            buf[1] |= IEEE802154_FCF_SRC_ADDR_LONG;
            for (int i = 7; i >= 0; i--) {
                buf[pos++] = src[i];
            }
            break;
        default:
            return 0;
    }

    /* return actual header length */
    return pos;
}

size_t ieee802154_get_frame_hdr_len(uint8_t *mhr)
{
    /* TODO: include security header implications */
    uint8_t tmp;
    size_t len = 3; /* 2 byte FCF, 1 byte sequence number */

    /* figure out address sizes */
    tmp = (mhr[1] & IEEE802154_FCF_DST_ADDR_MASK);
    if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
        len += 4;   /* 2 byte dst PAN + 2 byte dst short address */
    }
    else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
        len += 10;  /* 2 byte dst PAN + 2 byte dst long address */
    }
    else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
        return 0;
    }
    else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
        /* PAN compression, but no destination address => illegal state */
        return 0;
    }
    tmp = (mhr[1] & IEEE802154_FCF_SRC_ADDR_MASK);
    if (tmp == IEEE802154_FCF_SRC_ADDR_VOID) {
        return len;
    }
    else {
        if (!(mhr[0] & IEEE802154_FCF_PAN_COMP)) {
            len += 2;
        }
        if (tmp == IEEE802154_FCF_SRC_ADDR_SHORT) {
            return len + 2;
        }
        else if (tmp == IEEE802154_FCF_SRC_ADDR_LONG) {
            return len + 8;
        }
    }
    return 0;
}

int ieee802154_get_src(uint8_t *mhr, uint8_t *src)
{
    int offset = 3; /* FCF: 0-1, Seq: 2 */
    uint8_t tmp;

    assert((src != NULL));
    tmp = mhr[1] & IEEE802154_FCF_DST_ADDR_MASK;
    if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
        if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
        }
        offset += 4;
    }
    else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
        offset += 10;
    }
    else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
        return -EINVAL;
    }
    else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
        /* PAN compression, but no destination address => illegal state */
        return -EINVAL;
    }

    tmp = mhr[1] & IEEE802154_FCF_SRC_ADDR_MASK;
    if (tmp != IEEE802154_FCF_SRC_ADDR_VOID) {
        if (!(mhr[0] & IEEE802154_FCF_PAN_COMP)) {

        }
    }
    if (tmp == IEEE802154_FCF_SRC_ADDR_SHORT) {
        /* read src PAN and address in little endian */
        src[1] = mhr[offset++];
        src[0] = mhr[offset++];
        return 2;
    }
    else if (tmp == IEEE802154_FCF_SRC_ADDR_LONG) {
        /* read src PAN and address in little endian */
        for (int i = 7; i >= 0; i--) {
            src[i] = mhr[offset++];
        }
        return 8;
    }
    else if (tmp != IEEE802154_FCF_SRC_ADDR_VOID) {
        return -EINVAL;
    }

    return 0;
}

int ieee802154_get_dst(const uint8_t *mhr, uint8_t *dst, le_uint16_t *dst_pan)
{
    int offset = 3; /* FCF: 0-1, Seq: 2 */
    uint8_t tmp;

    assert((dst != NULL));
    tmp = mhr[1] & IEEE802154_FCF_DST_ADDR_MASK;
    if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
        /* read dst PAN and address in little endian */
        dst[1] = mhr[offset++];
        dst[0] = mhr[offset++];
        return 2;
    }
    else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
        for (int i = 7; i >= 0; i--) {
            dst[i] = mhr[offset++];
        }
        return 8;
    }
    else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
        return -EINVAL;
    }
    else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
        /* PAN compression, but no destination address => illegal state */
        return -EINVAL;
    }

    return 0;
}


/** @} */
