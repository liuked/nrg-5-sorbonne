#ifndef __API_H__
#define __API_H__

#include <stdint.h>

/*
 * This function is used by upper layer to send a data
 * packet to nso layer. If the data size exceed the MTU that
 * nso layer provided, this function will only send the first
 * MTU bytes of data.
 * --------------------------------------------------------
 * @buf: buffer to store a data packet who wants to be sent
 * @size: actual size of @buf
 * @return: how many bytes that are sent successfully;
 *          -1 means a error occurs;
 * */
int nso_send(uint8_t *buf, int size);

/*
 * This function is used by upper layer to receive a data 
 * packet from nso layer. If there is no data packet ready,
 * call to this function will be blocked until a data packet
 * arrive.
 * --------------------------------------------------------
 * @buf: buffer to store a received data packet
 * @size: maximum size of @buf
 * @return: actual data size of the received data packet;
 *          0 means TODO;
 *          -1 means a error occurs;
 * */
int nso_receive(uint8_t *buf, int size);

/*
 * This function is used by upper layer to check the MTU that
 * is supported by nso layer.
 * --------------------------------------------------------
 * @return: MTU;
 *          -1 means a error occurs;
 * */
int nso_get_mtu();

#endif
