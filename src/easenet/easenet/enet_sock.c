#include "enet_types.h"
#include "enet_sock.h"
#include "enet_sys.h"
#include "enet_packet.h"

int g_enet_sock_init = 0; // the sock module is initialized or not

/* In Windows System, we should init WSAStartup before we want 
 * to operate other socket operations
 */
#ifdef _WIN32
void enet_socket_init()
{
    if (!g_enet_sock_init) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(0x0202, &wsaData);
#endif
        g_enet_sock_init = 1;
    }
}
#else
#define enet_socket_init()
#endif

void enet_sockaddr_init(struct sockaddr_in* addr, int port, int address)
{
    if (!addr) return;
    addr->sin_addr.s_addr = address;
    addr->sin_port = htons((unsigned short)port);
    addr->sin_family = AF_INET;
}

void enet_sockaddr_init_broadcast(struct sockaddr_in* addr, int port)
{
    if (!addr) return;
    addr->sin_addr.s_addr = INADDR_BROADCAST;
    addr->sin_port = htons((unsigned short)port);
    addr->sin_family = AF_INET;
}

/** 
 * Check if the socket is writable
 * @description:
 *     null
 * @input: pointer of client handle
 * @return:
 *     - 1 means writable
 *     - 0 measn un-writable
 */
INTERNALCALL int enet_sock_is_writable(SOCKET fd)
{
    struct timeval tv = {0, 1};
    int rc;
    FD_SET set;

    FD_ZERO(&set);
    FD_SET(fd, &set);

    rc = select(fd+1, 0, &set, 0, &tv);
    if (FD_ISSET(fd, &set)) {
        return 1;
    }
    return 0;
}

/**
 * Bind the specified Port of tcp server
 * @description:
 *     null
 * @input: pointer of server
 * return: status
 */
lbs_status_t enet_tcp_server_bind_to_port(ENET_TCP_SERVER* server)
{
    int rc;

    rc = bind(server->fd, (const struct sockaddr*)(&server->addr), ADDR_LEN);
    if (rc == -1)
        return LBS_CODE_FAILED_TO_BIND;

    return LBS_CODE_OK;
}

lbs_status_t enet_udp_server_bind_to_port(ENET_UDP_OBJ* server)
{
    int rc = bind(server->fd, (const struct sockaddr*)(&server->addr), ADDR_LEN);

    if (rc == -1) {
        return LBS_CODE_FAILED_TO_BIND;
    }

    return LBS_CODE_OK;
}

void enet_tcp_server_socket_cleanup(ENET_TCP_SERVER* server)
{
    closesocket(server->fd);
    server->fd = 0;
    memset(&server->addr, 0, sizeof(struct sockaddr));
}

void enet_tcp_client_socket_cleanup(ENET_TCP_CLIENT* client)
{
    closesocket(client->fd);
    client->fd = 0;
    memset(&client->addr, 0, sizeof(struct sockaddr));
}

lbs_status_t enet_udp_create_socket(int port, ENET_UDP_OBJ* udp)
{
    int buf_len = ENET_DEFAULT_SOCKET_BUFFER;
    int res;

    enet_socket_init();

    udp->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp->fd == -1) {
        return LBS_CODE_WIN_ERROR;
    }
    
    res = setsockopt(udp->fd, SOL_SOCKET,SO_RCVBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 
    res = setsockopt(udp->fd, SOL_SOCKET,SO_SNDBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 

    enet_sockaddr_init(&udp->addr, port, ADDR_ANY);

    return LBS_CODE_OK;
}

lbs_status_t enet_tcp_server_create_socket(int port, ENET_TCP_SERVER* server)
{
    int buf_len = ENET_DEFAULT_SOCKET_BUFFER + sizeof(ENET_DATA_PACKET_HEAD) + sizeof(ENET_DATA_PACKET_TAIL);
    lbs_status_t rc;
    int res;

    enet_socket_init();

    server->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->fd == -1) {
        return LBS_CODE_WIN_ERROR;
    }
    
    res = setsockopt(server->fd, SOL_SOCKET,SO_RCVBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 

    res = setsockopt(server->fd, SOL_SOCKET,SO_SNDBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 

    enet_sockaddr_init(&server->addr, port, INADDR_ANY);

    rc = enet_tcp_server_bind_to_port(server);
    if (rc != LBS_CODE_OK) {
        enet_tcp_server_socket_cleanup(server);
        return rc;
    }

    return LBS_CODE_OK;
}

lbs_status_t enet_tcp_client_create_socket(int port, int addr, ENET_TCP_CLIENT* client)
{
    int buf_len = ENET_DEFAULT_SOCKET_BUFFER + sizeof(ENET_DATA_PACKET_HEAD) + sizeof(ENET_DATA_PACKET_TAIL);
    int res;

    if (port == 0 || client == 0)
        return LBS_CODE_INV_PARAM;

    enet_socket_init();

    client->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client->fd == -1) {
        return enet_error_code_convert(enet_get_sys_error());
    }
    
    res = setsockopt(client->fd, SOL_SOCKET,SO_RCVBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 
    res = setsockopt(client->fd, SOL_SOCKET,SO_SNDBUF, (const char*)&buf_len, sizeof(int));
    if (res != 0) {
        return enet_error_code_convert(enet_get_sys_error());
    } 

    enet_sockaddr_init(&client->addr, port, addr);

    return LBS_CODE_OK;
}

void enet_tcp_server_close_socket(ENET_TCP_SERVER* server)
{
    enet_tcp_server_socket_cleanup(server);
}

void enet_tcp_client_close_socket(ENET_TCP_CLIENT* client)
{
    enet_tcp_client_socket_cleanup(client);
}

lbs_status_t enet_tcp_socket_send(
    SOCKET fd,
    void* buf,
    int size,
    int use_packet
    )
{
    int rc;
    int sended = 0;
    int final_size = ENET_DEFAULT_SOCKET_BUFFER;
    char* p;
    unsigned char output[ENET_DEFAULT_SOCKET_BUFFER];

    if (use_packet) {
        rc = enet_pack_data((unsigned char*)buf, size, output, &final_size);
        if (rc != LBS_CODE_OK)
            return rc;
        p = (char*)output;
    } else {
        p = (char*)buf;
        final_size = size;
    }

resend:
    rc = send(fd, p, final_size, 0);
    if (rc == -1) {
        rc = enet_get_sys_error();
        if (rc == WSAEWOULDBLOCK) {
            Sleep(1);
        } else {
            return enet_error_code_convert(rc);
        }
        
        if (!enet_sock_is_writable(fd)) {
            return LBS_CODE_SOCKET_UNWRITABLE;
        }
        goto resend;
    }

    sended += rc;
    if (sended < final_size) {
        p += rc;
        goto resend;
    }

    return LBS_CODE_OK;
}

lbs_status_t enet_tcp_socket_recv(
    SOCKET fd,
    unsigned char* buffer,
    unsigned buffer_size,
    unsigned* received_size
    )
{
    int recv_size;
    int received = 0;

    //do {
    recv_size = recv(fd, (char*)buffer + received, buffer_size - received, 0);
    if (recv_size == -1) {
        return enet_error_code_convert(enet_get_sys_error());
    }
    if (recv_size == 0) {
        return LBS_CODE_REMOTE_DISCONNECT;
    }

    received += recv_size;
    //} while (recv_size);

    *received_size = received;

    return LBS_CODE_OK;
}

lbs_status_t enet_udp_socket_send(SOCKET fd, void* buf, int size)
{
    return LBS_CODE_OK;
}

lbs_status_t enet_udp_socket_broadcast(SOCKET fd, int port, void* buf, int size)
{
    int is_enable = 1;
    int res;
    int len;
    lbs_status_t rc;
    struct sockaddr_in addr;

    res = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&is_enable, sizeof(int));
    if (res == -1) {
        return enet_error_code_convert(enet_get_sys_error());
    }
    
    enet_sockaddr_init_broadcast(&addr, port);

resend:
    len = sendto(fd, (char*)buf, size, 0, (const struct sockaddr*)&addr, ADDR_LEN);
    if (len == -1) {
        rc = enet_error_code_convert(enet_get_sys_error());
        goto bail;
    }
    if (len < size) {
        size -= len;
        buf = ((char*)buf)+len;
        Sleep(10);
        goto resend;
    }

    rc = LBS_CODE_OK;

bail:
    is_enable = 0;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&is_enable, sizeof(int));

    return rc;
}
