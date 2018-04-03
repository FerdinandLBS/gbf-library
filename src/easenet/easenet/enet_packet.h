#ifndef _ENET_PACKAGE_H_
#define _ENET_PACKAGE_H_

#include "enet_types.h"

/* EASE NET DATA PACKAGE FUNCTIONS
 * 
 * These functions are designed to against TCP package merging.
 * But this is a applicatoin level data processing. This means
 * both sides of the communication should use this library.
 * 
 * Also caller should enable or disable this feature when creating
 * objects. Default behavior is disable
 */

typedef struct __enet_packet_head__ {
    int magic;
    int magic_reverse;
    unsigned length;
    int length_reverse;
}ENET_DATA_PACKET_HEAD;

typedef struct __enet_packet_tail__ {
    int magic;
    int check;
}ENET_DATA_PACKET_TAIL;

typedef struct __packet_unit {
    unsigned char data[ENET_DEFAULT_SOCKET_BUFFER];
    unsigned size;
}PACKET_UNIT;

#define HEAD_SIZE sizeof(ENET_DATA_PACKET_HEAD)
#define TAIL_SIZE sizeof(ENET_DATA_PACKET_TAIL)

lbs_status_t enet_init_packet_pool();

PACKET_UNIT* enet_alloc_packet_unit();

void enet_free_packet_unit(PACKET_UNIT* unit);

lbs_status_t enet_pack_data(
    unsigned char* in,
    int size,
    unsigned char* out,
    int* out_size
    );

lbs_status_t enet_unpack_data(
    unsigned char* in,
    unsigned size,
    unsigned char* buffer,
    unsigned* buffer_size,
    unsigned char* out,
    unsigned* out_size
    );


#endif //_ENET_PACKAGE_H_
