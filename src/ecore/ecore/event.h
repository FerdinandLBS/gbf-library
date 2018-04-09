#ifndef __ECORE_EVENT_H__
#define __ECORE_EVENT_H__

typedef enum __ecore_event_id__ {
    ECORE_EVENT_NOPE = 0,
    ECORE_EVENT_POST_MESSAGE,
    ECORE_EVENT_TCP_SERVER_CLIENT_RECV,
    ECORE_EVENT_TCP_SERVER_CLIENT_DISCONNECT,
    ECORE_EVENT_TCP_CLIENT_RECV,
}ECORE_EVENT_ID;

typedef struct __ecore_message_st__ {
    int msg;
    int module_id;
    void* data;
}ECORE_MESSAGE;

void init_event();
int post_event(ECORE_MESSAGE* msg);

#endif //__ENET_EVENT_H__
