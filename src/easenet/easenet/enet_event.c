#include "enet_event.h"
#include "lbs_thread.h"
#include "lbs_rwlock.h"
#include "enet_sock.h"
#include "enet_packet.h"
#include "enet_sys.h"


#include <stdio.h>
#include <assert.h>

//#define USE_STACK

lbs_thread_t __enet_event_thread = 0;
lbs_lock_t* __enet_event_lock = 0;
int __event_iterator = 0;
int __event_head = 0;
int __event_tail = 0;
int __event_is_ready = 0;
int __event_item_count = 0;
int __event_total_handler = 0;

typedef struct __enet_event_info {
    void* object;
    void* param1;
    void* param2;
    ENET_EVENT_ID id;
}ENET_EVENT_INFO;

#ifdef USE_STACK
ENET_EVENT_INFO __enet_event_db[ENET_DEFAULT_EVENT_DB_SIZE] = {0};
#else
ENET_EVENT_INFO* __enet_event_db = 0;
#endif

#define enet_event_db_lock() lbs_rwlock_wlock(__enet_event_lock)
#define enet_event_db_unlock() lbs_rwlock_unlock(__enet_event_lock)

/**
 * Get event from the head, if there is no, pending
 * @description:
 *     null
 * @output: pointer of event info
 * @return: true or false
 */
INTERNALCALL int enet_pop_event(ENET_EVENT_INFO* event_info)
{
    int result = 0;

continue_waiting:
    while (__event_item_count == 0) {
        /*
         * For a real server, Sleep(0) or SwitchToThread() is OK. We dont consider the CPU too much.
         * But in normal APP, Sleep(1) is better and releases much more CPU resource.
         */
        enet_sleep(1);
        //SwitchToThread();
    }

    enet_event_db_lock();

	if (__event_item_count == 0) {
		enet_event_db_unlock();
		goto continue_waiting;
	}

    event_info->id = __enet_event_db[__event_head].id;
    event_info->object = __enet_event_db[__event_head].object;
    event_info->param1 = __enet_event_db[__event_head].param1;
    event_info->param2 = __enet_event_db[__event_head].param2;

    __enet_event_db[__event_head].id = ENET_EVENT_NOPE;

    if (__event_item_count > 1)
        if (__event_head < ENET_DEFAULT_EVENT_DB_SIZE - 1) {
            __event_head++;
        } else {
            __event_head = 0;
        }
    else
        assert(__event_head == __event_tail);

    result = 1;
    __event_item_count--;

    enet_event_db_unlock();

    return result;
}

extern lbs_status_t enet_tcp_server_process_recv(
    enet_handle_t* server,
    SOCKET fd,
    void* buf, 
    int size
    );
extern void enet_process_client_receive(
    enet_handle_t* client,
    PACKET_UNIT* unit
    );

extern lbs_status_t enet_tcp_server_remove_client(
    enet_handle_t* server,
    SOCKET fd
    );

/**
 * Process all the events in this thread
 * @description:
 *     null
 * @input: param;
 * @return: thread exit code
 */
unsigned LBS_THREAD_CALL enet_event_handler(void* param)
{
    ENET_EVENT_INFO event_info;
    lbs_status_t st;

    while (1) {
        enet_pop_event(&event_info);

        switch (event_info.id) {
        case ENET_EVENT_CONNECT_REQUEST:
            {
                enet_handle_t* server = (enet_handle_t*)event_info.object;
                st = enet_new_tcp_server_client_object(server);
                if (st != LBS_CODE_OK) {
                    /* We should log error here */
                    ;
                }
            }
            break;
        case ENET_EVENT_TCP_SERVER_CLIENT_RECV:
            {
                PACKET_UNIT* packet = (PACKET_UNIT*)event_info.param1;
                enet_handle_t* server = (enet_handle_t*)event_info.object;
                SOCKET fd = (SOCKET)event_info.param2;

                enet_tcp_server_process_recv(server, fd, packet->data, packet->size);
                enet_free_packet_unit(packet);
            }
            break;
        case ENET_EVENT_TCP_SERVER_CLIENT_DISCONNECT:
            {
                int err = (int)event_info.param1;
                SOCKET fd = (SOCKET)event_info.param2;
                enet_handle_t* server = (enet_handle_t*)event_info.object;

                enet_tcp_server_remove_client(server, fd);
            }
            break;
        case ENET_EVENT_TCP_CLIENT_RECV:
            {
                PACKET_UNIT* packet = (PACKET_UNIT*)event_info.param1;
                enet_handle_t* client = (enet_handle_t*)event_info.object;
                
                enet_process_client_receive(client, packet);
                enet_free_packet_unit(packet);
            }
            break;
        default:
            ;//printf("invalid message type, count %d\n", __event_total_handler);
        }
    }

    return 0;
}

lbs_status_t enet_init_event_handler()
{
    int result;
    lbs_status_t st;

    if (__event_is_ready) {
        return 0;
    }
    
    result = lbs_rwlock_init(&__enet_event_lock);
    if (result != 0) {
        return LBS_CODE_NO_MEMORY;
    }

#ifndef USE_STACK
    __enet_event_db = (ENET_EVENT_INFO*)malloc(sizeof(ENET_EVENT_INFO)*ENET_DEFAULT_EVENT_DB_SIZE);
    if (__enet_event_db == 0) {
        lbs_rwlock_destroy(&__enet_event_lock);
        __enet_event_lock = 0;
        return LBS_CODE_NO_MEMORY;
    }
#endif

    __enet_event_db[0].id = ENET_EVENT_NOPE;

    st = lbs_create_thread(0, enet_event_handler, 0, &__enet_event_thread);
    if (st != LBS_CODE_OK) {
        lbs_rwlock_destroy(&__enet_event_lock);
        __enet_event_lock = 0;

#ifndef USE_STACK
        free(__enet_event_db);
        __enet_event_db = 0;
#endif
        return st;
    }

    __event_is_ready = 1;

    return LBS_CODE_OK;
}

lbs_status_t enet_post_event(void* object, void* param1, void* param2,  ENET_EVENT_ID event_id)
{
    lbs_status_t st = LBS_CODE_INTERNAL_ERR;

    if (!object || event_id == ENET_EVENT_NOPE)
        return LBS_CODE_INV_PARAM;

    enet_event_db_lock();

    /* Data base is full */
    if (__event_item_count == ENET_DEFAULT_EVENT_DB_SIZE)
        goto escape;
    
    /* If the event count eq zero, tail should not move */
    if (__event_item_count != 0)
        if (__event_tail < ENET_DEFAULT_EVENT_DB_SIZE - 1) {
            __event_tail++;
        } else {
            __event_tail = 0;
        }

    __enet_event_db[__event_tail].id = event_id;
    __enet_event_db[__event_tail].object = object;
    __enet_event_db[__event_tail].param1 = param1;
    __enet_event_db[__event_tail].param2 = param2;
    __event_item_count++;
    __event_total_handler++;

    st = LBS_CODE_OK;

escape:
    enet_event_db_unlock();

    return st;
}


