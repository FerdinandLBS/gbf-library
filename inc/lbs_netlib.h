#ifndef _LBS_NETLIB_H_
#define _LBS_NETLIB_H_


typedef unsigned long lbs_netlib_handle_t; // socket handle, server/client
typedef unsigned long lbs_netlib_client_t; // client id, used for server_send

typedef int (*read_callback)(lbs_netlib_client_t client, void* data, int size);
typedef int (*error_callback)(lbs_netlib_client_t client, int error);

typedef enum _lbs_socklib_status {
    /* Operation successful */
    LBS_NETLIB_OK = 0,

    /* Invalid parameters have been passed*/
    LBS_NETLIB_INV_PARAM = 1,

    /* Not enough memory */
    LBS_NETLIB_NO_MEMORY = 2,

    /* Request has been denied by remote server/client */
    LBS_NETLIB_REMOTE_DENY = 3,

    /* Server is not started. Can not do other actions */
    LBS_NETLIB_SERV_NOT_START = 4,

    /* Client cannot connect to remove server */
    LBS_NETLIB_CLIENT_NOT_CONN = 5,

    /* Operation time out */
    LBS_NETLIB_TIME_OUT = 6,

    /* Invalid socket handle has been passed */
    LBS_NETLIB_INV_HANDLE = 7,

    /* Too many socket objects have been created */
    LBS_NETLIB_TOO_MANY_OBJ = 8,

    /* Connection is break up. Please reconnect */
    LBS_NETLIB_NEED_RECONN = 9,

    /* Socket has been shutdown. Please reconnect */
    LBS_NETLIB_CONN_RESET = 10,

    /* Cannot send because remote socket does not receive message */
    LBS_NETLIB_CANNOT_SEND = 11,

    /* Cannot create more thread */
    LBS_NETLIB_CANNOT_CREATE_THREAD = 12,

    /* Object to be operated is corrupt. Maybe you can recreate it. */
    LBS_NETLIB_OJB_CORRUPT = 13,

    /* The server has been started. You cannot change the configuration */
    LBS_NETLIB_SERVER_STARTED = 14,

    /* The server is offline or connect has broken up */
    LBS_NETLIB_REMOTE_DISCONN = 15,

    /* Invalid port value */
    LBS_NETLIB_INV_PORT_VALUE = 16,

    /* Retry later */
    LBS_NETLIB_NEED_RETRY = 17,

    /* Cannot find client in server client list */
    LBS_NETLIB_CLIENT_NOT_FOUND = 18,
    /*
    LBS_NETLIB_
    LBS_NETLIB_
    LBS_NETLIB_
    LBS_NETLIB_
    */
    LBS_NETLIB_UNKNOWN_ERR,
}lbs_netlib_status_t;

#define LBS_NETLIB_INV_HANDLE 0

/* Error callback information */
#define LBS_NETLIT_ERR_EVENT_CONN    0 // A client connected
#define LBS_NETLIB_ERR_EVENT_DISCONN 1 // A client\Server disconnected
#define LBS_NETLIB_ERR_EVENT_NO_MEM  2 // Not enough memory
#define LBS_NETLIB_ERR_EVENT_NO_RESC 3 // Not enough system resource
#define LBS_NETLIB_ERR_EVENT_UNKNOWN 4 // Unknown error


#ifdef __cplusplus
extern "C" {
#endif

    /***************************************************************
     * Server Operatoins:
     *     Create TCP server
     */
    lbs_netlib_handle_t lbs_netlib_server_create(
        int port,
        int active,
        read_callback  read_cb,
        error_callback error_cb
        );

    /**
     * Server Operatoins:
     *     Start TCP server
     */
    lbs_netlib_status_t lbs_netlib_server_start(
        lbs_netlib_handle_t server
        );

    /**
     * Server Operatoins:
     *     To kickoff a TCP client from connection list
     */
    lbs_netlib_status_t lbs_netlib_server_kickoff_client(
        lbs_netlib_handle_t server,
        lbs_netlib_client_t client
        );

    /**
     * Server Operatoins:
     *     Ask server send data to a specified client.
     *     You can get a client id in server receive callback func
     *     if this client has send data to server.
     */
    lbs_netlib_status_t lbs_netlib_server_send_to_client(
        lbs_netlib_client_t client, 
        unsigned char* data, 
        int size
        );

    /**
     * Server Operatoins:
     *     Stop TCP server
     *     This function will drop all connections and close all
     *     threads. Call start server func to restart it.
     */
    lbs_netlib_status_t lbs_netlib_server_stop(
        lbs_netlib_handle_t server, 
        int sync
        );

    /**
     * Server Operatoins:
     *     Destroy and release a TCP server obj
     */
    lbs_netlib_status_t lbs_netlib_server_release(
        lbs_netlib_handle_t server
        );



    /***************************************************************
     * Client operation:
     *     Create client
     */
    lbs_netlib_handle_t lbs_netlib_client_create(
        int port, 
        int addr,
        int active,
        read_callback  read_cb,
        error_callback error_cb
        );

    /**
     * Client operation:
     *     Client connect to server
     */
    lbs_netlib_status_t lbs_netlib_client_connect(
        lbs_netlib_handle_t client,
        int retry_count
        );

    /**
     * Client operation:
     *     Client disconnect from server
     */
    lbs_netlib_status_t lbs_netlib_client_disconnect(
        lbs_netlib_handle_t client
        );

    /**
     * Client operation:
     *     Destroy and release a client obj
     */
    lbs_netlib_status_t lbs_netlib_client_release(
        lbs_netlib_handle_t client
        );

    /**
     * Client operation:
     *     Ask client send data to the server it has connected to.
     */
    lbs_netlib_status_t lbs_netlib_client_send(
        lbs_netlib_handle_t client, 
        unsigned char* data, 
        int size
        );

    
    /***************************************************************
     * UDP socket operations:
     *     Create UDP socket object
     */
    lbs_netlib_handle_t lbs_netlib_udp_create(
        int bind_port, 
        int target_port, 
        int target_addr, 
        read_callback read_cb,
        error_callback error_cb
        );

    /**
     * UDP socket operations:
     *     Send data to UDP target
     */
    lbs_netlib_status_t lbs_netlib_udp_send(
        lbs_netlib_handle_t udp_handle,
        lbs_netlib_client_t udp_client,
        void* data,
        int size
        );
    

    /***************************************************************
     * Common operation:
     *     Set callback functions for socket object
     */
    lbs_netlib_status_t lbs_netlib_set_callback(
        lbs_netlib_handle_t object,
        read_callback  read_cb,
        error_callback error_cb
        );

    /**
     * Common operation:
     *     Get the last error. Only call this after a func which doesnt
     *     return a value failed.
     */
    lbs_netlib_status_t lbs_netlib_get_last_error();

#ifdef __cplusplus
};
#endif


#endif //_LBS_NETLIB_H_
