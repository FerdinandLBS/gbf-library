#include "enet_types.h"
#include "enet_handle.h"
#include "enet_sock.h"
#include "enet_sys.h"
#include "enet_event.h"
#include <lbs_thread.h>

/**
 * Get UDP object from handle
 * @description:
 *     the magic number will be checked
 * @input: pointer of UDP handle
 * @return: pointer of UDP object
 */
INTERNALCALL ENET_UDP_OBJ* enet_get_udp_object(enet_handle_t* udp)
{
    ENET_UDP_OBJ* out = (ENET_UDP_OBJ*)(udp->obj);

    if (out->magic != 0x3244C) {
        return 0;
    }
    return out;
}

/**
 * Create new UDP object
 * @description:
 *     null
 * @input: port to bind
 * @input: parameter
 * @input: receive callback function
 * @return: pointer of UDP object
 */
INTERNALCALL ENET_UDP_OBJ* enet_new_udp_object(int port, void* param, cb_udp_received recv_cb)
{
    ENET_UDP_OBJ* out = (ENET_UDP_OBJ*)calloc(1, sizeof(ENET_UDP_OBJ));

    if (out == 0) 
        return out;

    out->magic = 0x3244C;
    out->recv_cb = recv_cb;
    out->param = param;
    
    enet_udp_create_socket(port, out);

    return out;
}

/**
 * Delete UDP object
 * @description:
 *     Socket will be closed, listen thread will be terminated
 * @input: pointer of UDP object
 * @return: null
 */
INTERNALCALL void enet_delete_udp_object(ENET_UDP_OBJ* udp)
{
    if (udp->fd != -1 && udp->fd != 0) {
        closesocket(udp->fd);
    }

    if (udp->listen_thread) {
        lbs_terminate_thread(udp->listen_thread, 0);
        udp->listen_thread = 0;
    }

    free(udp);
}


unsigned long LBS_THREAD_CALL upd_receive_thread(void* param)
{
    enet_handle_t* udp_handle = (enet_handle_t*)param;
    ENET_UDP_OBJ* udp = enet_get_udp_object(udp_handle);
    FD_SET readSet;
    unsigned char buf[ENET_DEFAULT_SOCKET_BUFFER] = "";
    ENET_UDP_INFO udp_info = {0};
    struct timeval time_st = {1, 0};
    int size;

    /* Here we don't check the return value of udp. Becuase no issue could happen here 
     * Also we dont handle the multiple thread cases.
     */

    FD_ZERO(&readSet);
    FD_SET(udp->fd, &readSet);

    while (1) {
        SOCKET fd = 0;
        cb_udp_received recv_callback = 0;
        void* caller_param;
        int len = sizeof(struct sockaddr);

        int rc = select(0, &readSet, 0, 0, &time_st);
        if (rc == -1 || rc == 0)
            continue;

        if (enet_handle_rlock(udp_handle)) {
            /* this handle maybe released. exit the thread */
            return 0;
        }
        
        fd = udp->fd;
        recv_callback = udp->recv_cb;
        caller_param = udp->param;

        enet_handle_unlock(udp_handle);

        size = recvfrom(fd, (char*)buf, ENET_DEFAULT_SOCKET_BUFFER, 0, &udp_info.addr_info, &len);
        if (size == -1)
            continue;

        if (recv_callback) {
            recv_callback(udp_handle, buf, size, caller_param);
        }
    }

}

INTERNALCALL lbs_status_t enet_udp_create_listen_thread(enet_handle_t* udp_handle)
{
    ENET_UDP_OBJ* udp = enet_get_udp_object(udp_handle);

    udp->listen_thread = CreateThread(0, 0, upd_receive_thread, udp_handle, 0, 0);
    if (udp->listen_thread == INVALID_HANDLE_VALUE) {
        return LBS_CODE_WIN_ERROR;
    }

    return LBS_CODE_OK;
}


lbs_status_t ENET_CALLCONV enet_new_udp(
    int port,
    void* param,
    cb_udp_received recv_cb,
    enet_handle_t** udp
    )
{
    enet_handle_t* out;
    ENET_UDP_OBJ* object;
    lbs_status_t rc;
    int res;

    if (port == 0 || udp == 0) {
        return LBS_CODE_INV_PARAM;
    }
    
    rc = enet_init_event_handler();
    if (rc)
        return rc;

    object = enet_new_udp_object(port, param, recv_cb);
    if (object == 0)
        return LBS_CODE_NO_MEMORY;

    out = enet_new_handle_ex(object, 1);
    if (out == 0) {
        rc = LBS_CODE_NO_MEMORY;
        goto bail;
    }

    res = bind(object->fd, (struct sockaddr*)&object->addr, sizeof(struct sockaddr));
    if (res == -1) {
        rc = LBS_CODE_SOCK_ERROR;
        goto bail;
    }

    rc = enet_udp_create_listen_thread(out);
    if (rc != LBS_CODE_OK) {
        goto bail;
    }

    *udp = out;

    return LBS_CODE_OK;

bail:
    if (object) {
        if (object->fd) {
            closesocket(object->fd);
        }
        free(object);
    }
    if (out) {
        enet_delete_handle(out);
    }
    return rc;
}

lbs_status_t ENET_CALLCONV enet_udp_set_target(
    enet_handle_t* udp_handle,
    int ip,
    int port
    )
{
    lbs_status_t rc;
    ENET_UDP_OBJ* udp;

    if (ip == 0 || port == 0) {
        return LBS_CODE_INV_PARAM;
    }

    rc = enet_handle_rlock(udp_handle);
    if (rc != LBS_CODE_OK)
        return rc;

    udp = enet_get_udp_object(udp_handle);

    enet_sockaddr_init(&udp->addr, port, ip);

    enet_handle_unlock(udp_handle);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_udp_send(
    enet_handle_t* udp_handle,
    void* data,
    int size
    )
{
    lbs_status_t rc;
    ENET_UDP_OBJ* udp;
    struct sockaddr addr_to;
    SOCKET fd;
    cb_udp_received recv_cb;
    int len;

    if (data == 0 || size == 0) {
        return LBS_CODE_INV_PARAM;
    }
    if (size > ENET_DEFAULT_SOCKET_BUFFER) {
        return LBS_CODE_BUFFER_TOO_LARGE;
    }

    rc = enet_handle_rlock(udp_handle);
    if (rc != LBS_CODE_OK)
        return rc;

    udp = enet_get_udp_object(udp_handle);
    fd = udp->fd;
    recv_cb  = udp->recv_cb;
    memcpy(&addr_to, &udp->addr, sizeof(struct sockaddr));
    
    enet_handle_unlock(udp_handle);

    len = sendto(fd, (char*)data, size, 0, (struct sockaddr*)&addr_to, sizeof(struct sockaddr));
    if (len == -1) {
        return LBS_CODE_SOCK_ERROR;
    }
    if (len == 0) {
        return LBS_CODE_SOCK_ERROR;
    }

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_udp_broadcast(enet_handle_t* udp_handle, int port, void* data, int size)
{
    lbs_status_t rc;
    ENET_UDP_OBJ* udp;
    SOCKET fd;
    
    if (data == 0 || size == 0)
        return LBS_CODE_INV_PARAM;
    if (size > ENET_DEFAULT_SOCKET_BUFFER) {
        return LBS_CODE_BUFFER_TOO_LARGE;
    }

    rc = enet_handle_rlock(udp_handle);
    if (rc != LBS_CODE_OK)
        return rc;

    udp = enet_get_udp_object(udp_handle);
    if (udp == 0) {
        enet_handle_unlock(udp_handle);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    fd = udp->fd;

    enet_handle_unlock(udp_handle);

    rc = enet_udp_socket_broadcast(fd, port, data, size);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_udp_send_to(
    enet_handle_t* udp,
    int ip,
    int port,
    void* data,
    int size
    )
{
    lbs_status_t rc;

    if (ip == 0 || port == 0) {
        return LBS_CODE_INV_PARAM;
    }
    if (size > ENET_DEFAULT_SOCKET_BUFFER) {
        return LBS_CODE_BUFFER_TOO_LARGE;
    }

    rc = enet_handle_rlock(udp);
    if (rc != LBS_CODE_OK)
        return rc;

    enet_handle_unlock(udp);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_udp_send_back(enet_handle_t* udp_handle, ENET_UDP_INFO* info, void* data, int size)
{
    lbs_status_t rc;
    ENET_UDP_OBJ* udp;
    
    if (size == 0 || data == 0)
        return LBS_CODE_INV_PARAM;
    if (size > ENET_DEFAULT_SOCKET_BUFFER) {
        return LBS_CODE_BUFFER_TOO_LARGE;
    }

    rc = enet_handle_rlock(udp_handle);
    if (rc != LBS_CODE_OK)
        return rc;

    udp = enet_get_udp_object(udp_handle);
    if (udp == 0) {
        enet_handle_unlock(udp_handle);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }



    enet_handle_unlock(udp_handle);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_delete_udp(enet_handle_t* udp_handle)
{
    lbs_status_t rc;
    ENET_UDP_OBJ* udp;

    rc = enet_handle_wlock(udp_handle);
    if (rc != LBS_CODE_OK)
        return rc;

    udp = enet_get_udp_object(udp_handle);
    if (udp == 0) {
        enet_handle_unlock(udp_handle);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    enet_delete_udp_object(udp);

    udp_handle->obj = 0;

    enet_handle_unlock(udp_handle);

    enet_delete_handle(udp_handle);
    
    return LBS_CODE_OK;
}
