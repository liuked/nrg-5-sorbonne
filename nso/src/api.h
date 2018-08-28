#ifndef __API_H__
#define __API_H__

#include <stdint.h>
#include "nso_common.h"

#define ERROR_BASE -1
#define ERROR_NOT_READY (ERROR_BASE - 1)

typedef struct __attribute__((packed)) {
    uint8_t addr[DEV_ID_WIDTH];
}nso_addr_t;

/*
 * This function is used by upper layer to send a data
 * packet to nso layer. If the data size exceed the MTU that
 * nso layer provided, this function will only send the first
 * MTU bytes of data.
 * --------------------------------------------------------
 * @buf: buffer to store a data packet who wants to be sent.
 *       Note, the data packet is copied by nso layer.
 *       Thus, the memory of @buffer is not used by nso layer
 *       anymore after return from this function call.
 * @size: actual size of @buf
 * @dest: destination address. @dest can be NULL, when this 
 *        packet is sent to base station.
 * @return: how many bytes that are sent successfully;
 *          negative number means a error occurs.
 * */
int nso_send(uint8_t *buf, int size, nso_addr_t *dest);

/*
 * This function is used by upper layer to receive a data 
 * packet from nso layer. If there is no data packet ready,
 * call to this function will be blocked until a data packet
 * arrive.
 * --------------------------------------------------------
 * @buf: buffer to store a received data packet. The data packet
 *       is copied from nso layer memory to the memory of @buffer
 * @size: maximum size of @buf
 * @src: return the source address of the received packet. It
 *       can be NULL, if the source address is not useful.
 * @dst: return the destination address of the received packet.
 *       It can be NULL, if the destination address is not
 *       useful.
 * @return: actual data size of the received data packet;
 *          0 means that #TODO#;
 *          negative number means a error occurs.
 * */
int nso_receive(uint8_t *buf, int size, nso_addr_t *src, nso_addr_t *dst);

/*
 * This function is used by upper layer to check the MTU that
 * is supported by nso layer.
 * --------------------------------------------------------
 * @return: MTU;
 *          negative number means a error occurs;
 * */
int nso_get_mtu();

#endif
