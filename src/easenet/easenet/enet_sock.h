#ifndef _EASENET_SOCK_H_
#define _EASENET_SOCK_H_

#define ADDR_LEN (sizeof(struct sockaddr))

/**
 * Init socket functions
 * @description:
 *     Always call it before you create an object handle
 * @return: null
 */
void enet_socket_init();

/**
 * Init sock address struct
 * @description:
 *     null
 * @output: pointer of sock address struct
 * @input: port
 * @input: address
 * @return: null
 */
void enet_sockaddr_init(struct sockaddr_in* addr, int port, int address);

/**
 * Init sock address struct
 * @description:
 *     null
 * @input: pointer of sock address struct
 * @input: port
 * @return: null
 */
void enet_sockaddr_init_broadcast(struct sockaddr_in* addr, int port);

/**
 * Create socket for TCP server
 * @description:
 *     null
 * @input: port
 * @input: pointer of server
 * @return: status
 */
lbs_status_t enet_tcp_server_create_socket(int port, ENET_TCP_SERVER* server);

/**
 * Create socket for TCP client
 * @description:
 *     null
 * @input: port of server
 * @input: address of server
 * @input: pointer of client
 * @return: status
 */
lbs_status_t enet_tcp_client_create_socket(int port, int addr, ENET_TCP_CLIENT* client);

/**
 * Create socket for UDP object
 * @description:
 *     null
 * @input: port of server
 * @input: pointer of UDP
 * @return: status
 */
lbs_status_t enet_udp_create_socket(int port, ENET_UDP_OBJ* udp);

void enet_tcp_server_close_socket(ENET_TCP_SERVER* server);
void enet_tcp_client_close_socket(ENET_TCP_CLIENT* client);

/**
 * Send data to a connected socket
 * @description:
 *     null
 * @input: socket
 * @input: buffer
 * @input: size of buffer
 * @input: use internal packet or not
 * @return: status
 */
lbs_status_t enet_tcp_socket_send(
    SOCKET fd,
    void* buf,
    int size,
    int use_packet
    );

/**
 * Receive raw data from one socket
 * @descritpion:
 *     receive all data once from the buffer.
 * @input: socket
 * @input: pointer of receive buffer
 * @input: buffer size
 * @input: pointer of received size
 * @return: status
 */
lbs_status_t enet_tcp_socket_recv(
    SOCKET fd,
    unsigned char* buffer,
    unsigned buffer_size,
    unsigned* received_size
    );

lbs_status_t enet_udp_socket_send(SOCKET fd, void* buf, int size);

/**
 * Broadcast data
 * @description:
 *     null
 * @input: socket
 * @input: port of client
 * @input: buffer
 * @input: size of buffer
 * @return: status
 */
lbs_status_t enet_udp_socket_broadcast(SOCKET fd, int port, void* buf, int size);

/**
 * Create TCP server client object
 * @description:
 *     Accept inside the function. Only call it when listen thread is triggerd
 * @input: pointer of server handle
 * @return: status
 */
lbs_status_t enet_new_tcp_server_client_object(
    enet_handle_t* server
    );

#endif
