#ifndef _EASENET_TYPES_H_
#define _EASENET_TYPES_H_

#define FD_SETSIZE 128

#ifdef _WIN32
#include <Winsock2.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")
#else

#include <pthread.h>

#endif

#include <thp.h>
#include <rbtree.h>
#include <lbs_thread.h>

/* Handle of ease net object */
typedef struct __ease_net_handle enet_handle_t;
typedef struct __ease_net_tcp_server_client ENET_TCP_SERVER_CLIENT;
typedef struct __ease_net_tcp_client ENET_TCP_CLIENT;
typedef struct __ease_net_udp_info ENET_UDP_INFO;

#define ENET_CALLCONV _stdcall
#define ENET_CALLBACK _stdcall

typedef int (ENET_CALLBACK* cb_tcp_server_connected)(enet_handle_t* which_server, ENET_TCP_SERVER_CLIENT* client, void* param);
typedef int (ENET_CALLBACK* cb_tcp_server_disconnected)(enet_handle_t* which_server, ENET_TCP_SERVER_CLIENT* client, void* param);
typedef int (ENET_CALLBACK* cb_tcp_server_received)(enet_handle_t* which_server, ENET_TCP_SERVER_CLIENT* client, void* data, int size, void* param);
typedef int (ENET_CALLBACK* cb_tcp_client_received)(enet_handle_t* client, void* data, int size, void* param);
typedef int (ENET_CALLBACK* cb_tcp_client_disconnected)(enet_handle_t* client, void* param);
typedef int (ENET_CALLBACK* cb_udp_received)(enet_handle_t* udp, void* data, int size, void* param);

/*
 * Object handle package
 */
struct __ease_net_handle {
    void* obj;
    void* lock;
    int alive;
    int refcnt; // atom operations are necessary
    enet_handle_t* next;
    enet_handle_t* prev;
};

/*
 * TCP server
 *
 * Entity of tcp server.
 */
typedef struct __ease_net_tcp_server ENET_TCP_SERVER;
struct __ease_net_tcp_server {
    int magic; // 0x3324A
    int status;
    int select_timeout; // for select()

    void* param;
#ifdef _WIN32
    SOCKET fd;
#else
    socket fd;
#endif
    int flag;

    struct sockaddr_in addr;
    int connections;

    lbs_thread_t listener;

    thp_handle_t* worker_pool;

    /* Callback functions */
    cb_tcp_server_connected conn_cb;
    cb_tcp_server_disconnected disc_cb;
    cb_tcp_server_received  recv_cb;

    /* Tree of clients */
    rbtree_t* clients_tree;
};

/*
 * The object of clients
 * Both our client object and connected clients of server use this
 * structure to save information.
 * Client object will be encapsulated by handle.
 * Clients of server will not be.
 */
struct __ease_net_tcp_server_client {
#ifdef _WIN32
    SOCKET fd;
#else
    int fd;
#endif

    int is_writable; // if this client is writable or not
    struct sockaddr_in addr; // the address information of the client

    int is_waiting;
    unsigned buffer_size;
    unsigned char buffer[1];
};

struct __ease_net_tcp_client {
    int magic; //0x3244B
    void* param;

#ifdef _WIN32
    SOCKET fd;
#else
    int fd;
#endif

    int flag;

    struct sockaddr_in addr; // the address information of the client
    cb_tcp_client_received recv_cb; // the callback function of caller

    /* the receive thread handle */
    lbs_thread_t recv_thread;

    unsigned buffer_size;
    unsigned char buffer[1];
};

/* UDP OBJECT */
typedef struct __ease_net_udp {
    int magic; //0x3324C
    void* param;
#ifdef _WIN32
    SOCKET fd;
#else
    int fd;
#endif

    lbs_thread_t listen_thread;

    struct sockaddr_in addr; // local addr info
    cb_udp_received recv_cb;
}ENET_UDP_OBJ;

struct __ease_net_udp_info {
    void* upd_object;
    struct sockaddr addr_info;
};


#ifndef INTERNALCALL
#define INTERNALCALL static
#endif

#ifndef _WIN32
#define TRUE 1
#define FALSE 0
#endif

#define ENET_SERVER_RUN       0
#define ENET_SERVER_SUSPEND   1
#define ENET_SERVER_TERMINATE 2

#define ENET_DEFAULT_SELECT_TIMEOUT 2500
#define ENET_DEFAULT_SOCKET_BUFFER  64*1024
#define ENET_DEFAULT_EVENT_DB_SIZE  1000
#define ENET_DEFAULT_PACKET_POOL_SIZE 100

/* Flags of tcp server*/
#define ENET_SERVER_ATTR_USE_PACKET 0x00010000
#define ENET_SERVER_ATTR_USE_BUFFER 0x00020000

/* Flags of tcp client */
#define ENET_CLIENT_ATTR_USE_PACKET 0x00010000
#define ENET_CLIENT_ATTR_USE_BUFFER 0x00020000

#endif
