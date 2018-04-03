#ifndef __ENET_EVENT_H__
#define __ENET_EVENT_H__

#include "enet_types.h"

typedef enum __enet_event_id__ {
    ENET_EVENT_NOPE = 0,
    ENET_EVENT_CONNECT_REQUEST,
    ENET_EVENT_TCP_SERVER_CLIENT_RECV,
    ENET_EVENT_TCP_SERVER_CLIENT_DISCONNECT,
    ENET_EVENT_TCP_CLIENT_RECV,
}ENET_EVENT_ID;

lbs_status_t enet_init_event_handler();

lbs_status_t enet_post_event(void* object, void* param1, void* param2,  ENET_EVENT_ID event_id);


#endif //__ENET_EVENT_H__
