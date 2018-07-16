#include "enet_types.h"
#include <lbs_rwlock.h>
#include "enet_handle.h"
#include "enet_sock.h"
#include "enet_event.h"
#include "enet_packet.h"
#include "enet_sys.h"

/**
 * Get the client object according to the handle
 * @description:
 *     the magic number will be checked
 * @input: pointer of client handle 
 * @return: pointer of client object
 */
INTERNALCALL ENET_TCP_CLIENT* enet_get_tcp_client_object(enet_handle_t* client)
{
    ENET_TCP_CLIENT* out = (ENET_TCP_CLIENT*)(client->obj);

    if (out->magic != 0x3244B) {
        out = 0;
    }

    return out;
}

INTERNALCALL lbs_status_t enet_get_tcp_client_attr(
    enet_handle_t* client,
    void** param,
    SOCKET* fd,
    int* flag,
    cb_tcp_client_received* recv_cb,
    ENET_TCP_CLIENT** object_out
    )
{
    ENET_TCP_CLIENT* object;
    lbs_status_t status;

    status = enet_handle_rlock(client);
    if (status != LBS_CODE_OK) {
        return status;
    }

    object = enet_get_tcp_client_object(client);
    if (object == 0) {
        enet_handle_unlock(client);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    if (param)
        *param = object->param;
    if (fd)
        *fd = object->fd;
    if (flag)
        *flag = object->flag;
    if (recv_cb)
        *recv_cb = object->recv_cb;
    if (object_out)
        *object_out = object;

    enet_handle_unlock(client);

    return LBS_CODE_OK;
}

void enet_process_client_receive(
    enet_handle_t* client,
    PACKET_UNIT* unit
    )
{
    int flag;
    void* param;
    unsigned real_size = ENET_DEFAULT_SOCKET_BUFFER;
    unsigned char real_data[ENET_DEFAULT_SOCKET_BUFFER];
    unsigned char* p;
    cb_tcp_client_received recv_callback;
    lbs_status_t st;
    ENET_TCP_CLIENT* object;

    st = enet_get_tcp_client_attr(
        client, &param, 0, &flag, &recv_callback, &object
        );
    if (st != LBS_CODE_OK)
        return;

    if (flag & ENET_CLIENT_ATTR_USE_PACKET) {
        st = enet_unpack_data(
            unit->data, unit->size, 
            object->buffer, &object->buffer_size,
            real_data, &real_size
            );
        if (st != 0) return;

        p = real_data;
    } else {
        p = unit->data;
        real_size = unit->size;
    }

    if (recv_callback) {
        recv_callback(client, p, real_size, param);
    }
}

unsigned int LBS_THREAD_CALL tcp_client_recv_thread(void* param)
{
    enet_handle_t* client  = (enet_handle_t*)param;
    SOCKET fd;
    lbs_status_t st;
    int endurance = 2;
    ENET_TCP_CLIENT* object;

    /* Duplicate attributes */
    st = enet_get_tcp_client_attr(client, 0, &fd, 0, 0, &object);
    if (st != LBS_CODE_OK)
        return st;

    /* Here we want the thread terminate itself if the socket is invalid at
     * some moment. So that we dont need to terminate manually or set a 
     * flag to tell it terminating.
     */
    while (endurance > 0) {
        PACKET_UNIT* unit;

        unit = enet_alloc_packet_unit();
        if (unit == 0) {
            enet_sleep(0);
            continue;
        }

        st = enet_tcp_socket_recv(fd, unit->data, ENET_DEFAULT_SOCKET_BUFFER, &unit->size);
        if (st != LBS_CODE_OK) {
            enet_sleep(10);
            endurance--;
            continue; 
        }

        enet_post_event(client, unit, 0, ENET_EVENT_TCP_CLIENT_RECV);
    }
    
    return 0;
}

INTERNALCALL lbs_status_t enet_tcp_client_create_recv_thread(enet_handle_t* client)
{
    ENET_TCP_CLIENT* object = enet_get_tcp_client_object(client);
    lbs_status_t rc;

#ifdef _WIN32
    rc = lbs_create_thread(client, tcp_client_recv_thread, 0, &object->recv_thread);

#else
#endif
    return rc;
}

INTERNALCALL lbs_status_t enet_delete_tcp_client_object(ENET_TCP_CLIENT* client_object)
{
    if (client_object) {
        if (client_object->fd) {
            closesocket(client_object->fd);
        }

        /* Dont wait here now. The thread should be closed if socket is closed */
        //lbs_wait_thread(client_object->recv_thread, 4000);

        free(client_object);
    }

    return LBS_CODE_OK;
}

INTERNALCALL lbs_status_t enet_new_tcp_client_object(
    int port,
    int ip,
    int flag,
    void* param,
    cb_tcp_client_received recv_cb,
    ENET_TCP_CLIENT** client_object
    )
{
    lbs_status_t rc;
    int size;
    ENET_TCP_CLIENT* out;

    if (flag & ENET_CLIENT_ATTR_USE_PACKET) {
        size = sizeof(ENET_TCP_CLIENT) + HEAD_SIZE + TAIL_SIZE + ENET_DEFAULT_SOCKET_BUFFER;
    } else if (flag & ENET_CLIENT_ATTR_USE_BUFFER) {
        size = sizeof(ENET_TCP_CLIENT) + ENET_DEFAULT_SOCKET_BUFFER;
    } else {
        size = sizeof(ENET_TCP_CLIENT);
    }

    out = (ENET_TCP_CLIENT*)calloc(1, size);
    if (out == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    out->param = param;
    out->magic = 0x3244B;
    out->flag = flag&0xffff0000;
    out->buffer_size = 0;
    
    rc = enet_tcp_client_create_socket(port, ip, out);
    if (rc != LBS_CODE_OK) {
        free(out);
        return rc;
    }

    if (recv_cb) {
        out->recv_cb = recv_cb;
    }

    *client_object = out;

    return 0;
}

INTERNALCALL lbs_status_t enet_set_socket_nonblock(
    SOCKET fd
    )
{
    lbs_status_t rc = LBS_CODE_OK;
    unsigned code;

#ifdef _WIN32
    u_long arg = 1;

    code = ioctlsocket(fd, FIONBIO, &arg);
#else
    code = fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif
    if (code != 0)
        rc = enet_error_code_convert(enet_get_sys_error());

    return rc;
}

INTERNALCALL lbs_status_t enet_set_socket_blocking(
    SOCKET fd
    )
{
    lbs_status_t rc = LBS_CODE_OK;
    unsigned code;

#ifdef _WIN32
    u_long arg = 0;
    code = ioctlsocket(fd, FIONBIO, &arg);
#else
    code = fcntl(sockfd, F_SETFL, ~O_NONBLOCK);
#endif
    if (code != 0)
        rc = enet_error_code_convert(enet_get_sys_error());

    return rc;
}

lbs_status_t ENET_CALLCONV enet_new_tcp_client(
    int ip, 
    int port,
    int flag,
    void* param,
    cb_tcp_client_received recv_callback, 
    enet_handle_t** handle
    )
{
    ENET_TCP_CLIENT* client_object = 0;
    enet_handle_t* out = 0;
    lbs_status_t rc;

    if (ip == 0 || port == 0) {
        return LBS_CODE_INV_PARAM;
    }
    
    rc = enet_init_event_handler();
    if (rc)
        return rc;

    rc = enet_init_packet_pool();
    if (rc)
        return rc;

    rc = enet_new_tcp_client_object(port, ip, flag, param, recv_callback, &client_object);
    if (rc != LBS_CODE_OK)
        goto bail;

    out = enet_new_handle_ex(client_object, 1);
    if(out == 0) {
        rc = LBS_CODE_NO_MEMORY;
        goto bail;
    }

    *handle = out;

    return LBS_CODE_OK;
bail:
    enet_delete_tcp_client_object(client_object);
    enet_delete_handle(out);

    return rc ;
}

lbs_status_t ENET_CALLCONV enet_tcp_client_connect(enet_handle_t* client, int timeout)
{
    ENET_TCP_CLIENT* object;
    int rc;
    int try_count = 5;
    fd_set write_set;
    struct timeval time_st = {2, 0};

    FD_ZERO(&write_set);

    if (timeout > 0) {
        time_st.tv_sec = timeout/1000;
        time_st.tv_usec = (timeout%1000)*1000;
    }

    if (enet_handle_rlock(client)) {
        return LBS_CODE_INV_HANDLE;
    }

    object = enet_get_tcp_client_object(client);
    if (object == 0) {
        enet_handle_unlock(client);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    if (object->fd == 0) {
        rc = enet_tcp_client_create_socket(object->addr.sin_port, object->addr.sin_addr.S_un.S_addr, object);
        if (rc != LBS_CODE_OK) {
            enet_handle_unlock(client);
            return rc;
        }
    }
    
    enet_set_socket_nonblock(object->fd);

    connect(object->fd, (struct sockaddr*)(&(object->addr)), sizeof(struct sockaddr));

    FD_SET(object->fd, &write_set);

    rc = select(0, 0, &write_set, 0, &time_st);
    
    enet_set_socket_blocking(object->fd);

    if (rc <= 0) {
        unsigned i = enet_get_sys_error();
        lbs_status_t st;

        closesocket(object->fd);
        object->fd = 0;
        st = lbs_wait_thread(object->recv_thread, 4000);
        if (st == LBS_CODE_TIME_OUT) {
            lbs_terminate_thread(object->recv_thread, 0);
        }

        enet_handle_unlock(client);

        if (rc == 0)
            return LBS_CODE_TIME_OUT;
        else
            return enet_error_code_convert(i);
    }
    
    rc = enet_tcp_client_create_recv_thread(client);
    if (rc != LBS_CODE_OK) {
        enet_handle_unlock(client);
        return rc;
    }

    enet_handle_unlock(client);

    return rc;
}

lbs_status_t ENET_CALLCONV enet_tcp_client_disconnect(enet_handle_t* handle)
{
    lbs_status_t rc;
    int rc1;
    ENET_TCP_CLIENT* object;

    rc = enet_handle_wlock(handle);
    if (rc != LBS_CODE_OK)
        return rc;

    object = enet_get_tcp_client_object(handle);
    if (object == 0) {
        enet_handle_unlock(handle);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    rc1 = closesocket(object->fd);
    if (rc1 == -1) {
        enet_handle_unlock(handle);
        return LBS_CODE_SOCK_ERROR;
    }
    object->fd = 0;

    lbs_wait_thread(object->recv_thread, 4000);

    enet_handle_unlock(handle);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_delete_tcp_client(enet_handle_t* handle)
{
    lbs_status_t rc;
    ENET_TCP_CLIENT* object;

    rc = enet_handle_rlock(handle);
    if (rc != LBS_CODE_OK)
        return rc;

    object = enet_get_tcp_client_object(handle);
    if (object == 0) {
        enet_handle_unlock(handle);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    handle->obj = 0;

    enet_delete_tcp_client_object(object);

    enet_handle_unlock(handle);
    enet_delete_handle(handle);

    return rc;
}

lbs_status_t ENET_CALLCONV enet_tcp_client_send(enet_handle_t* handle, void* buf, int size)
{
    lbs_status_t rc;
    ENET_TCP_CLIENT* object;
    int resend = 0;
    int use_packet;

    if (buf == 0 || size == 0) {
        return LBS_CODE_INV_PARAM;
    }
    if (size > ENET_DEFAULT_SOCKET_BUFFER) {
        return LBS_CODE_BUFFER_TOO_LARGE;
    }

    rc = enet_handle_rlock(handle);
    if (rc != LBS_CODE_OK)
        return rc;

    object = enet_get_tcp_client_object(handle);
    if (object == 0) {
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }
    use_packet = object->flag & ENET_CLIENT_ATTR_USE_PACKET;

    enet_handle_unlock(handle);

retry:
    rc = enet_tcp_socket_send(object->fd, buf, size, use_packet);
    if (rc != LBS_CODE_OK && !resend) {
        if (LBS_CODE_OK == enet_tcp_client_connect(handle, 5000)) {
            resend = 1;
            goto retry;
        }
    }

    return rc;
}
