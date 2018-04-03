#include "enet_packet.h"
#include "lbs_rwlock.h"
#include "assert.h"

#define PACKET_USE_STACK

lbs_lock_t* __packet_pool_lock = 0;
unsigned __packet_pool_ready = 0;
unsigned __packet_pool_size = ENET_DEFAULT_PACKET_POOL_SIZE;

#ifdef PACKET_USE_STACK
PACKET_UNIT __packet_db[ENET_DEFAULT_PACKET_POOL_SIZE] = {0};
unsigned char __packet_state[ENET_DEFAULT_PACKET_POOL_SIZE] = {0};
unsigned __packet_free_list[ENET_DEFAULT_PACKET_POOL_SIZE] = {0};
#else
PACKET_UNIT* __packet_db = 0;
unsigned char* __packet_state = 0;
unsigned* __packet_free_list = 0;
#endif

int __packet_free_list_iterator = ENET_DEFAULT_PACKET_POOL_SIZE - 1;

INTERNALCALL int enet_get_packet_tail_check(
    ENET_DATA_PACKET_HEAD* head,
    ENET_DATA_PACKET_TAIL* tail
    )
{
    return head->magic^head->length^tail->magic;
}

INTERNALCALL int enet_is_head_valid(
    ENET_DATA_PACKET_HEAD* head
    )
{
    if (head->magic != 0x3244 ||
        head->magic_reverse != (head->magic^0xffffffff) ||
        head->length_reverse != (head->length^0xffffffff)
        )
    {
        return 0;
    }

    return 1;
}

lbs_status_t enet_init_packet_pool()
{
    unsigned i;
    lbs_status_t st;

    if (__packet_pool_ready == 1)
        return LBS_CODE_OK;

#ifdef PACKET_USE_STACK

#else
    __packet_db = (PACKET_UNIT*)calloc(1, sizeof(PACKET_UNIT)*__packet_pool_size);
    if (__packet_db == 0)
        goto bail;
    __packet_state = (unsigned char*)calloc(1, sizeof(unsigned char)*__packet_pool_size);
    if (__packet_state == 0)
        goto bail;
    __packet_free_list = (unsigned*)calloc(1, sizeof(unsigned)*__packet_pool_size);
    if (__packet_free_list == 0)
        goto bail;
#endif

    for (i = 0; i < __packet_pool_size; i++) {
        __packet_free_list[i] = i;
    }
    __packet_free_list_iterator = __packet_pool_size-1;

    st = lbs_rwlock_init(&__packet_pool_lock);
    if (st != LBS_CODE_OK)
        goto bail;

    __packet_pool_ready = 1;

    return LBS_CODE_OK;
bail:
    
#ifndef PACKET_USE_STACK
    if (__packet_db)
        free(__packet_db);
    __packet_db = 0;
    if (__packet_state)
        free(__packet_state);
    __packet_state = 0;
    if (__packet_free_list) 
        free(__packet_free_list);
    __packet_free_list = 0;
#endif

    return LBS_CODE_NO_MEMORY;
}

PACKET_UNIT* enet_alloc_packet_unit()
{
    PACKET_UNIT* out;

    lbs_rwlock_wlock(__packet_pool_lock);
	
    if (__packet_free_list_iterator == -1) {
		lbs_rwlock_unlock(__packet_pool_lock);
        return 0;
    }

    out = &__packet_db[__packet_free_list[__packet_free_list_iterator]];

    __packet_state[__packet_free_list[__packet_free_list_iterator]] = 1;
    __packet_free_list_iterator--;

    lbs_rwlock_unlock(__packet_pool_lock);

    return out;
}

void enet_free_packet_unit(PACKET_UNIT* unit)
{
    unsigned index;
    unsigned p1, p2, p3, p4;

    if (unit == 0) return;

    p1 = (unsigned)unit;
    p2 = (unsigned)__packet_db;
    p3 = p1-p2;
    p4 = sizeof(PACKET_UNIT);
    index = p3/p4;
#ifdef _DEBUG
    if (index < 0 || index >= __packet_pool_size || index == 0)
        assert(0);
#endif

    lbs_rwlock_wlock(__packet_pool_lock);

    __packet_free_list_iterator++;
#ifdef _DEBUG
    if (__packet_free_list_iterator >= __packet_pool_size)
        assert(0);
#endif

    __packet_state[index] = 0;
    __packet_free_list[__packet_free_list_iterator] = index;

    lbs_rwlock_unlock(__packet_pool_lock);
}

lbs_status_t enet_pack_data(
    unsigned char* in,
    int size,
    unsigned char* out,
    int* out_size
    )
{
    ENET_DATA_PACKET_HEAD head;
    ENET_DATA_PACKET_TAIL tail;

    head.magic = 0x3244;
    head.magic_reverse = head.magic^0xffffffff;
    head.length = size;
    head.length_reverse = head.length^0xffffffff;

    tail.magic = head.magic>>16 | head.magic<<16;
    tail.check = enet_get_packet_tail_check(&head, &tail);

    if (*out_size < HEAD_SIZE + size + TAIL_SIZE)
        assert(0);

    memcpy(out, &head, HEAD_SIZE);
    memcpy(out + HEAD_SIZE, in, size);
    memcpy(out + HEAD_SIZE + size, &tail, TAIL_SIZE);

    *out_size = HEAD_SIZE + TAIL_SIZE + size;

    return 0;
}

#define log(_log_) printf(_log_)
#ifdef log
#include <stdio.h>
#endif

lbs_status_t enet_unpack_data(
    unsigned char* in,
    unsigned size,
    unsigned char* buffer,
    unsigned* buffer_size,
    unsigned char* out,
    unsigned* out_size
    )
{
    ENET_DATA_PACKET_HEAD head;
    //ENET_DATA_PACKET_TAIL tail;

#ifdef _DEBUG
    assert(size <= ENET_DEFAULT_SOCKET_BUFFER && *buffer_size <= ENET_DEFAULT_SOCKET_BUFFER && *out_size <= ENET_DEFAULT_SOCKET_BUFFER);
#endif

    if (*buffer_size == 0 && size == 0) return LBS_CODE_UNCOMPLETE_PACKET;

    if (*buffer_size == 0) {
        if (size >= HEAD_SIZE) {
            memcpy(&head, in, HEAD_SIZE);
            if (!enet_is_head_valid(&head)) {
                return LBS_CODE_DATA_CORRUPT;
            }
        } else {
            memcpy(buffer, in, size);
            *buffer_size = size;
            return LBS_CODE_UNCOMPLETE_PACKET;
        }

        if (head.length + HEAD_SIZE + TAIL_SIZE <= size) {
            if (*out_size < head.length)
                assert(0);

            memcpy(out, in + HEAD_SIZE, head.length);
            *out_size = head.length;

            if (size - HEAD_SIZE - TAIL_SIZE > head.length) {
                memcpy(buffer, in + HEAD_SIZE + head.length, size - HEAD_SIZE - head.length);
                *buffer_size = size - HEAD_SIZE - head.length;
            }
            return LBS_CODE_OK;
        } else {
            /* packet merging happen. Data should not forward */
            memcpy(buffer, in, size);
            *buffer_size = size;
            *out_size = 0;
            return LBS_CODE_UNCOMPLETE_PACKET;
        }
    } else { // buffer_size > 0
        if (*buffer_size < HEAD_SIZE) {
            if (size + *buffer_size <= HEAD_SIZE) {
                memcpy(buffer+*buffer_size, in, size);
                *buffer_size += size;
                return LBS_CODE_UNCOMPLETE_PACKET;
            }
            memcpy(buffer+*buffer_size, in, HEAD_SIZE);
            *buffer_size += HEAD_SIZE;
            in += HEAD_SIZE;
            size -= HEAD_SIZE;
        }
        memcpy(&head, buffer, HEAD_SIZE);
        
        if (size + *buffer_size < head.length + HEAD_SIZE + TAIL_SIZE) {
            memcpy(buffer+*buffer_size, in, size);
            *buffer_size += size;
            *out_size = 0;
            return LBS_CODE_UNCOMPLETE_PACKET;
        }

        //size + *buffer_size >= head.length + HEAD_SIZE + TAIL_SIZE
        {
            /*
             * |- head -|                           |--tail--|
             * |---     |         BUFFER            |        ---| - - - in - - - |
             *          |- - - - - - - out - - - - -|
             */

            int i, j, k;
            /* 
             * |- head - |          | - - - in - - - - - - - - - |
             * |---    BUFFER    ---|                |--tail--|
             *           |- - - - - - - out - - - - -|
             */
            
            i = *buffer_size-HEAD_SIZE; // data in buffer except head
            j = head.length - i;
            k = size - j - TAIL_SIZE;

            if (j > 0) {
                memcpy(out, buffer + HEAD_SIZE, i);
                memcpy(out + i, in, j);
            }else
                memcpy(out, buffer + HEAD_SIZE, head.length);

            if (k > 0) {
                memcpy(buffer, in + j + TAIL_SIZE, k);
                *buffer_size = k;
            } else {
                *buffer_size = 0;
            }
            *out_size = head.length;
            //printf("[%llu]data complete %d bytes\n", GetTickCount64(), head.length);
            return LBS_CODE_OK;
        }
    }

    return LBS_CODE_OK;
}

