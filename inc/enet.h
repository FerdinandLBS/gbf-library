#ifndef __EASE_NET_H__
#define __EASE_NET_H__

#ifdef _WIN32

#ifdef _WINDOWS_
#error enet.h header file must be included before 'Windows.h'
#endif

#include <Winsock2.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

#endif

typedef unsigned int enet_status_t;

/*
 * Object handle
 */
struct __ease_net_handle {
    void* obj;
    void* lock;
    int alive;
    int refcnt;
    void* next;
    void* prev;
};

typedef struct __ease_net_handle enet_handle_t;
typedef struct __ease_net_tcp_client ENET_TCP_SERVER_CLIENT;
typedef struct __ease_net_udp_info ENET_UDP_INFO;

#define ENET_CALLCONV _stdcall
#define ENET_CALLBACK _stdcall

/**
 * TCP server callback function: client connected
 * @description:
 *     null
 * @input: Which server is connected
 * @input: client object
 * @input: parameter when server was created
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_tcp_server_connected)(
    enet_handle_t* which_server,
    ENET_TCP_SERVER_CLIENT* client,
    void* param
    );

/**
 * TCP server callback function: client disconnected
 * @description:
 *     null
 * @input: Which server is disconnected
 * @input: Which client
 * @input: parameter when server was created
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_tcp_server_disconnected)(
    enet_handle_t* which_server,
    ENET_TCP_SERVER_CLIENT* client,
    void* param
    );

/**
 * TCP server callback function: received from one client
 * @description:
 *     null
 * @input: Which server received
 * @input: Which client sended
 * @input: pointer of data
 * @input: size of data
 * @input: parameter when server was created
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_tcp_server_received)(
    enet_handle_t* which_server,
    ENET_TCP_SERVER_CLIENT* client,
    void* data,
    int size,
    void* param
    );

/**
 * TCP client callback function: received
 * @description:
 *     null
 * @input: client handle
 * @input: pointer of data
 * @input: size of data
 * @input: peremeter when client was created
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_tcp_client_received)(
    enet_handle_t* client,
    void* data, int size,
    void* param
    );

/**
 * TCP client callback function: disconnected from server
 * @description:
 *     null
 * @input: client handle
 * @input: parameter when client was created
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_tcp_client_disconnected)(
    enet_handle_t* client,
    void* param
    );

/**
 * UDP object receive callback function
 * @description
 *     null
 * @input: pointer of UDP
 * @input: pointer of data
 * @input: size of data
 * @input: parameter which is inputed when UDP creating
 * @return: status
 */
typedef int (ENET_CALLBACK* cb_udp_received)(
    enet_handle_t* udp,
    void* data,
    int size,
    void* param
    );

/*
 * TCP client of server.
 * A server would handle multiple client objects.
 * This object would be passed when created and received data
 */
struct __ease_net_tcp_client {
    SOCKET fd;
    int is_writable;
    struct sockaddr_in addr;

    unsigned char buffer[1];
};

/* Flags of tcp server*/
#define ENET_SERVER_ATTR_USE_PACKET 0x00010000
#define ENET_SERVER_ATTR_USE_BUFFER 0x00020000

/* Flags of tcp client */
#define ENET_CLIENT_ATTR_USE_PACKET 0x00010000
#define ENET_CLIENT_ATTR_USE_BUFFER 0x00020000

/* For enet_tcp_client_connect() */
#define ENET_CLIENT_CONNECT_WAIT_DEFAULT 5000 // 5 sec
#define ENET_CLIENT_CONNECT_WAIT_INFINITE -1  // infinite

/* opt parameter in enet_http_set */
#define ENET_HTTP_SET_HEADER 0
#define ENET_HTTP_SET_URL    1
#define ENET_HTTP_SET_BODY   2

/* opt parameter in enet_http_fetch() */
#define ENET_HTTP_GET_RAW    0
#define ENET_HTTP_GET_HEADER 1
#define ENET_HTTP_GET_CODE   2
#define ENET_HTTP_GET_STATUS 3
#define ENET_HTTP_GET_BODY   4
#define ENET_HTTP_GET_VERION 5
#define ENET_HTTP_GET_REQUEST 6

/* method parameter in enet_http_exec() */
#define ENET_HTTP_GET       0
#define ENET_HTTP_POST      1
#define ENET_HTTP_PUT       2
#define ENET_HTTP_DELETE    3

#ifdef __cplusplus
extern "C" {
#endif
    
    /********************************************************************
     *                                                                  *
     *                          TOOL  FUNCTIONS                         *
     *                                                                  *
     ********************************************************************/
    enet_status_t ENET_CALLCONV enet_http_new(const char* url, int* handle);
    enet_status_t ENET_CALLCONV enet_http_set(int handle, int opt, void* key, void* value);
    enet_status_t ENET_CALLCONV enet_http_exec(int handle, int method);
    enet_status_t ENET_CALLCONV enet_http_fetch(int handle, int opt, const char* key, void** data, int* size);
    enet_status_t ENET_CALLCONV enet_http_del(int handle);

    enet_status_t ENET_CALLCONV enet_http_post(int ip, int port, const char* url, const char* param, const char* content, char** output);
    enet_status_t ENET_CALLCONV enet_http_get(int ip, int port, const char* url, const char* param, char** output);

    void ENET_CALLCONV enet_http_free(void* p);

    int ENET_CALLCONV enet_name_to_ip(const char* name);
    
    /********************************************************************
     *                                                                  *
     *                        TCP SERVER FUNCTIONS                      *
     *                                                                  *
     ********************************************************************/

    /**
    * Create TCP Server
     * @description:
     *     If connection callback is null, all connections will be accepted.
     *     If receiving callback is null, all received data will be dropped.
     * @input: binding port.
     * @input: flags. Lower 16-bit is thread counts. If a zero is passed, default count
     *         10 will be used. Higher 16 bit is flag which is unused now.
     * @input: select() time out. in micro-second
     * @input: parameter you want to pass in callbacks
     * @input: connection request handle callback function
     * @input: server receiving callback function
     * @input: client disconnecting callback function
     * @output: TCP servesr handle
     * @return: status
     * @!!!NOTE: Never process too much work in server callbacks. It may effect the event engine.
     */
    enet_status_t ENET_CALLCONV enet_new_tcp_server(
        int port,
        int flag,
        int select_timeout,
        void* param,
        cb_tcp_server_connected conn_cb,
        cb_tcp_server_received  recv_cb,
        cb_tcp_server_disconnected disc_cb,
        enet_handle_t** server
        );

    /**
     * Start TCP Server
     * @description:
     *     Start all worker threads
     * @input: pointer of server handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_start_tcp_server(
        enet_handle_t* server
        );

    /**
     * Stop TCP Server
     * @description:
     *     Pause all the worker threads and stop receiving data.
     * @input: pointer of server handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_stop_tcp_server(
        enet_handle_t* server
        );

    /**
     * Delete TCP Server
     * @description:
     *     Remove the server object
     * @input: pointer of server handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_delete_tcp_server(
        enet_handle_t* server
        );

    /**
     * Disconnect one Client
     * @description:
     *     null
     * @input: the pointer of server handle
     * @input: pointer of client
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_disconnect_client_from_tcp_server(
        enet_handle_t* server,
        ENET_TCP_SERVER_CLIENT* client
        );

    /**
     * Send Data to Client
     * @description:
     *     Send data to a specified client. The client must connect already.
     * @input: the pointer of server handle
     * @input: the pointer of client
     * @input: pointer of the data
     * @input: size of the data
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_tcp_server_send_to_client(
        enet_handle_t* server,
        ENET_TCP_SERVER_CLIENT* client,
        void* data,
        int size
        );

    enet_status_t ENET_CALLCONV enet_tcp_server_wait_packet(
        enet_handle_t* server,
        ENET_TCP_SERVER_CLIENT* client,
        void* data,
        int size,
        int is_clean
        );

    /**
    * Get current clients count
    * @description:
    *     Just the count when you call. Not real-time number
     * @input: pointer of server handle
     * @output: count of clients
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_tcp_server_get_client_count(
        enet_handle_t* server,
        int* count
        );

    
    /********************************************************************
     *                                                                  *
     *                        TCP CLIENT FUNCTIONS                      *
     *                                                                  *
     ********************************************************************/
    /**
     * Create TCP client
     * @description:
     *     null
     * @input: IP address of server
     * @input: port of server
     * @input: flag
     * @input: parameter you want to pass to callbacks
     * @input: receive callback function
     * @output: pointer of handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_new_tcp_client(
        int ip,
        int port,
        int flag,
        void* param,
        cb_tcp_client_received recv_callback,
        enet_handle_t** handle
        );

    /**
     * Connect to server
     * @description:
     *     - Connect to the server you specified when creating the client. 
     *     - Resume all wokring threads
     * @input: pointer of client handle
     * @input: time limitation of operation (in ms). Valid only if positive
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_tcp_client_connect(
        enet_handle_t* handle,
        int timeout
        );

    /**
     * Disconnect from server
     * @description:
     *     Disconnect and suspend working threads
     * @input: pointer of client handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_tcp_client_disconnect(
        enet_handle_t* handle
        );

    /**
     * Delete the TCP client
     * @description:
     *     null
     * @input: pointer of client handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_delete_tcp_client(
        enet_handle_t* handle
        );

    /**
     * Send data to server
     * @description:
     *     null
     * @input: pointer of client handle
     * @input: pointer of buffer
     * @input: size of buffer
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_tcp_client_send(
        enet_handle_t* handle,
        void* buf,
        int size
        );
    

    
    /********************************************************************
     *                                                                  *
     *                        UDP FUNCTIONS                             *
     *                                                                  *
     ********************************************************************/
    /**
     * Create new UDP object
     * @description:
     *     null
     * @input: port to bind
     * @input: parameter you want to pass to callbacks
     * @input: receive callback function
     * @output: 2nd rank pointer of UDP handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_new_udp(
        int port, 
        void* param,
        cb_udp_received recv_cb,
        enet_handle_t** udp
        );

    /**
     * Select target for UDP
     * @description:
     *     null
     * @input: pointer of UDP handle
     * @input: IP address string
     * @input: port
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_udp_set_target(
        enet_handle_t* udp,
        int ip,
        int port
        );

    /**
     * Send to UDP target
     * @description:
     *     you must call 'enet_udp_set_target'
     * @input: pointer of UDP handle
     * @input: pointer of data
     * @input: size of data
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_udp_send(
        enet_handle_t* udp,
        void* data,
        int size
        );

    /**
     * Broadcast to all UDP in child-net
     * @description:
     *     null
     * @input: pointer of UDP handle
     * @input: port
     * @input: pointer of data
     * @input: size of data
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_udp_broadcast(
        enet_handle_t* udp,
        int port,
        void* data,
        int size
        );

    /**
     * Sent to a spesified UDP
     * @description:
     *     null
     * @input: pointer of UDP handle
     * @input: string of IP address
     * @input: port
     * @input: pointer of data
     * @input: size of data
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_udp_send_to(
        enet_handle_t* udp,
        int ip,
        int port,
        void* data,
        int size
        );

    /**
     * Send to UDP according UDP info
     * @description:
     *     you would get UDP in receive callback
     * @input: pointer of UDP handle
     * @input: pointer of UDP info
     * @input: pointer of data
     * @input: size of data
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_udp_send_back(
        enet_handle_t* upd,
        ENET_UDP_INFO* info,
        void* data,
        int size
        );

    /**
     * Delete UDP object
     * @description:
     *     null
     * @input: pointer of UDP handle
     * @return: status
     */
    enet_status_t ENET_CALLCONV enet_delete_udp(
        enet_handle_t* udp
        );

#ifdef __cplusplus
};
#endif

/*
 * Error codes
 * Start from zero. Zero means OK
 */
#define ENET_CODE_OK            0 // Process successfully 
#define ENET_CODE_INV_PARAM     1 // An invalid parameter is inputed
#define ENET_CODE_NOT_FOUND     2 // Specified item cannot be found
#define ENET_CODE_NO_MEMORY     3 // This process is out of memory
#define ENET_CODE_ALREADY_EXIST 4 // Same item already exists, cannot create more
#define ENET_CODE_DENIED        5 // operation request is denied
#define ENET_CODE_INV_HANDLE    6 // the input handle is not valid
#define ENET_CODE_INV_FLAG      7 // the input flag is not valid
#define ENET_CODE_HANDLE_TYPE_MISMATCH 8 // wrong handle type
#define ENET_CODE_TIME_OUT      9 // operation out of time
#define ENET_CODE_DATA_CORRUPT  10 // data corrupt

// BST
#define ENET_CODE_INV_KEY    305 // invalid key value
#define ENET_CODE_EMPTY_TREE 306 // tree is empty

/* Net */
#define ENET_CODE_SENDING_UNCOMPLETE  1001 // Not all datas are send
#define ENET_CODE_SOCKET_UNWRITABLE   1002 // The socket is not writable
#define ENET_CODE_SERVER_TERMINATED   1003 // The server is closed
#define ENET_CODE_CLIENT_DISCONNECTED 1004
#define ENET_CODE_FAILED_TO_BIND      1005
#define ENET_CODE_FAILED_TO_CONNECT   1006
#define ENET_CODE_TOO_MANY_SOCKETS    1007
#define ENET_CODE_BUFFER_OVERFLOW     1008 // Buffer is too large
#define ENET_CODE_NETWORK_ISSUE       1009
#define ENET_CODE_REMOTE_DISCONNECT   1010
#define ENET_CODE_REMOTE_REFUESD      1011
#define ENET_CODE_BUFFER_TOO_LARGE    1012
#define ENET_CODE_UNCOMPLETE_PACKAGE  1013

/* Thread pool */
#define ENET_CODE_WATCHER_NOT_READY 3001
#define ENET_CODE_WATCHER_EXIST     3002
#define ENET_CODE_SUSPEND_BY_OTHERS 3003
#define ENET_CODE_NO_TASK           3004


#define ENET_CODE_INTERNAL_ERR -1
#define ENET_CODE_WIN_ERROR    -2
#define ENET_CODE_SOCK_ERROR   -3

#endif // __EASE_NET_H__


