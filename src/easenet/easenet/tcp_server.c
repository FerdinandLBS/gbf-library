#include "enet_types.h"
#include "enet_handle.h"
#include "enet_sock.h"
#include "enet_packet.h"
#include "enet_sys.h"
#include "enet_event.h"
#include <lbs_rwlock.h>
#include <lbs_thread.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

/**
 * Copy first fd_set to other three
 * @description:
 *     null
 * @input: source set
 * @output: target 1
 * @output: target 2
 * @output: target 3
 * @return: null
 */
INTERNALCALL void enet_fd_copy(FD_SET* src, FD_SET* dst1, FD_SET* dst2, FD_SET* dst3)
{
    int size = sizeof(FD_SET);
    unsigned char* p1 = (unsigned char*)src, *p2 = (unsigned char*)dst1, *p3 =(unsigned char*)dst2, *p4 =(unsigned char*)dst3;

    while (size) {
        size--;
        p4[size] = p3[size] = p2[size] = p1[size];
    }
}

/**
 * Get the server object according to the handle
 * @description:
 *     null
 * @input: pointer of server handle 
 * @return: pointer of server object
 */
INTERNALCALL ENET_TCP_SERVER* enet_get_tcp_server_object(enet_handle_t* server)
{
    ENET_TCP_SERVER* out = (ENET_TCP_SERVER*)(server->obj);

    if (out->magic == 0x3244A)
        return out;

    return 0;
}

/**
 * Get tcp server socket, the listening socket
 * @description:
 *     lock inside
 * @input: server handle
 * @output: socket
 * @return: status
 */
INTERNALCALL lbs_status_t enet_get_tcp_server_attr(
    enet_handle_t* server, 
    SOCKET* fd, 
    unsigned* flag,
    int* select_timeout,
    int* connections,
    cb_tcp_server_connected* conn_cb,
    cb_tcp_server_disconnected* disc_cb,
    cb_tcp_server_received* recv_cb,
    void** param
    )
{
    ENET_TCP_SERVER* obj;
    lbs_status_t st;

    st = enet_handle_rlock(server);
    if (st)
        return st;

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        st = LBS_CODE_HANDLE_TYPE_MISMATCH;
        goto escape;
    }

    if (fd)
        *fd = obj->fd;
    if (flag)
        *flag = obj->flag;
    if (select_timeout)
        *select_timeout = obj->select_timeout;
    if (connections)
        *connections = obj->connections;
    if (conn_cb)
        *conn_cb = obj->conn_cb;
    if (disc_cb)
        *disc_cb = obj->disc_cb;
    if (recv_cb)
        *recv_cb = obj->recv_cb;
    if (param)
        *param = obj->param;

    st = LBS_CODE_OK;

escape:
    enet_handle_unlock(server);

    return st;
}

#define enet_get_tcp_server_socket(__server, __fd) enet_get_tcp_server_attr(__server, __fd, 0,0,0,0,0,0,0)
#define enet_get_tcp_server_flag(__server, __flag) enet_get_tcp_server_attr(__server, 0, __flag, 0,0,0,0,0,0)

/**
 * Delete the TCP server client object
 * @description:
 *     The socket will be closed
 * @input: the pointer of client
 * @return: status
 */
INTERNALCALL lbs_status_t enet_delete_tcp_server_client_object(ENET_TCP_SERVER_CLIENT* client_object)
{
    if (client_object) {
        if (client_object->fd) {
            closesocket(client_object->fd);
        }

        free(client_object);
    }

    return LBS_CODE_OK;
}

/**
 * Add client to TCP server
 * @description:
 *     -insert the object to the client tree.
 *     -lock inside
 * @input: pointer of server handle
 * @input: pointer of client object
 * @return: status
 */
INTERNALCALL lbs_status_t enet_add_client_to_server(enet_handle_t* server, ENET_TCP_SERVER_CLIENT* client_obj)
{
    lbs_status_t rc;
    ENET_TCP_SERVER* object;
    ENET_TCP_SERVER_CLIENT* old_client = 0;

    rc = enet_handle_wlock(server);
    if (rc != LBS_CODE_OK) {
        return rc;
    }

    object = enet_get_tcp_server_object(server);

    rc = rbt_insert(object->clients_tree, client_obj->fd, client_obj, LBS_TRUE, &old_client);
    if (rc != LBS_CODE_OK) {
        enet_handle_unlock(server);
        return rc;
    }
    if (old_client) {
        free(old_client);
        enet_handle_unlock(server);
        return rc;
    }

    rc = thp_deliver_task(object->worker_pool, (void*)client_obj->fd, sizeof(ENET_TCP_SERVER_CLIENT), 0);
    if (rc != LBS_CODE_OK) {
        rbt_remove(object->clients_tree, client_obj->fd, &old_client);
        if (old_client) {
            enet_delete_tcp_server_client_object(old_client);
        }
        enet_handle_unlock(server);
        return rc;
    }

    enet_handle_unlock(server);

    return rc;
}

INTERNALCALL void enet_duplicate_server_client_without_data(
    const ENET_TCP_SERVER_CLIENT* src,
    ENET_TCP_SERVER_CLIENT* dst
    )
{
    memcpy(dst, src, sizeof(ENET_TCP_SERVER_CLIENT));
}

INTERNALCALL lbs_bool_t is_accpetable(SOCKET fd)
{
    lbs_bool_t res = LBS_TRUE;
    FD_SET fdset;
    int result;
    struct timeval tm = {0, 5000};

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    /* Suspending until the request comes */
    result = select(0, &fdset, 0, 0, &tm);
    if (result == 0 || result == -1) {
        return LBS_FALSE;
    }

    if (!FD_ISSET(fd, &fdset)) {
        return LBS_FALSE;
    }

    return LBS_TRUE;
}

/**
 * Create TCP server client object
 * @description:
 *     Accept inside the function. Only call it when listen thread is triggerd
 * @input: pointer of server handle
 * @return: status
 */
lbs_status_t enet_new_tcp_server_client_object(
    enet_handle_t* server
    )
{
    unsigned size;
    unsigned flag;
    lbs_status_t st;
    int addr_len = sizeof(struct sockaddr);
    SOCKET fd;
    ENET_TCP_SERVER_CLIENT* out;
    cb_tcp_server_connected conn_cb;
    void* param;
    
    st = enet_get_tcp_server_attr(server, &fd, &flag, 0,0, &conn_cb, 0,0, &param);
    if (st)
        return st;

    if (flag & ENET_SERVER_ATTR_USE_PACKET) {
        size = sizeof(ENET_TCP_SERVER_CLIENT) + HEAD_SIZE + TAIL_SIZE + ENET_DEFAULT_SOCKET_BUFFER;
    } else if (flag & ENET_SERVER_ATTR_USE_BUFFER) {
        size = sizeof(ENET_TCP_SERVER_CLIENT) + ENET_DEFAULT_SOCKET_BUFFER;
    } else  {
        size = sizeof(ENET_TCP_SERVER_CLIENT);
    }
    
    out = (ENET_TCP_SERVER_CLIENT*)calloc(1, size);
    if (out == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    /* Here we check the socket is still accpetable. otherwise we should return 
     * to avoid system blocking
     */
    if (!is_accpetable(fd)) {
        free(out);
        return LBS_CODE_OK;
    }

    out->fd = accept(fd, (struct sockaddr*)&(out->addr), &addr_len);
    if (out->fd == -1) {
        enet_delete_tcp_server_client_object(out);
        return enet_error_code_convert(enet_get_sys_error());
    }

    st = enet_add_client_to_server(server, out);

    if (st != LBS_CODE_OK) {
        enet_delete_tcp_server_client_object(out);
    }

    if (conn_cb) {
        conn_cb(server, out, param);
    }

    return st;
}

/**
 * Search for the client object according to the fd
 * @description:
 *     null
 * @input: pointer of server
 * @input: file descriptor
 * @return: pointer of client object
 */
INTERNALCALL ENET_TCP_SERVER_CLIENT* enet_find_client_in_tcp_server(enet_handle_t* server, SOCKET fd)
{
    ENET_TCP_SERVER* obj = enet_get_tcp_server_object(server);
    ENET_TCP_SERVER_CLIENT* out = 0;
    rbtree_node_t* node = 0;

    node = rbt_search(obj->clients_tree, fd);

    if (node) {
        out = (ENET_TCP_SERVER_CLIENT*)node->data;
    }

    return out;
}


INTERNALCALL lbs_status_t enet_tcp_server_process_recv_ok(
    enet_handle_t* server,
    SOCKET fd,
    FD_SET* set,
    void* buf, 
    int size
    )
{
    lbs_status_t rc;
    cb_tcp_server_received receive_callback;
    ENET_TCP_SERVER_CLIENT* client;
    ENET_TCP_SERVER* obj;
    void* param;
    int use_packet = 0;
    unsigned real_size = ENET_DEFAULT_SOCKET_BUFFER;
    unsigned char real_data[ENET_DEFAULT_SOCKET_BUFFER];

    if (enet_handle_rlock(server)) {
        FD_CLR(fd, set);
        return LBS_CODE_SERVER_TERMINATED;
    }

    /* Duplicate attributes */
    obj = (ENET_TCP_SERVER*)server->obj;
    receive_callback = obj->recv_cb;
    client = enet_find_client_in_tcp_server(server, fd);
    param = obj->param;
    if (obj->flag & ENET_SERVER_ATTR_USE_PACKET) {
        use_packet = 1;
    }

    enet_handle_unlock(server);

    if (client == 0) {
        closesocket(fd);
        FD_CLR(fd, set);
        return LBS_CODE_CLIENT_DISCONNECTED;
    }

    if (use_packet) {
        while (rc == LBS_CODE_OK) {
            rc = enet_unpack_data((unsigned char*)buf, size, client->buffer, &client->buffer_size, real_data, &real_size);
            if (rc)
                return rc;
            if (receive_callback != 0) {
                receive_callback(server, client, real_data, real_size, param);
            }
            buf = 0; size = 0; real_size = ENET_DEFAULT_SOCKET_BUFFER;
        }
    } else {
        if (receive_callback != 0) {
            receive_callback(server, client, buf, size, param);
        }
    }

    return LBS_CODE_OK;
}

lbs_status_t enet_tcp_server_process_recv(
    enet_handle_t* server,
    SOCKET fd,
    void* buf, 
    int size
    )
{
    lbs_status_t rc = LBS_CODE_OK;
    cb_tcp_server_received receive_callback;
    ENET_TCP_SERVER_CLIENT* client;
    ENET_TCP_SERVER* obj;
    void* param;
    int use_packet = 0;
    int use_buffer = 0;
    unsigned real_size = ENET_DEFAULT_SOCKET_BUFFER;
    unsigned char real_data[ENET_DEFAULT_SOCKET_BUFFER];
    unsigned char* p;

    if (enet_handle_rlock(server)) {
        return LBS_CODE_SERVER_TERMINATED;
    }

    /* Duplicate attributes */
    obj = (ENET_TCP_SERVER*)server->obj;
    receive_callback = obj->recv_cb;
    client = enet_find_client_in_tcp_server(server, fd);
    param = obj->param;
    if (obj->flag & ENET_SERVER_ATTR_USE_PACKET) {
        use_packet = 1;
    }
    if (obj->flag & ENET_SERVER_ATTR_USE_BUFFER) {
        use_buffer = 1;
    }

    enet_handle_unlock(server);

    if (client == 0) {
        closesocket(fd);
        return LBS_CODE_CLIENT_DISCONNECTED;
    }

    if (use_packet) {
        while (rc == LBS_CODE_OK) {
            rc = enet_unpack_data((unsigned char*)buf, size, client->buffer, &client->buffer_size, real_data, &real_size);
            if (rc) {
                //printf("server unpack %d bytes data, status: %d\n", size, rc);
                return rc;
            }
            p = real_data;
            if (receive_callback != 0) {
                receive_callback(server, client, p, real_size, param);
            }
            buf = 0; size = 0; real_size = ENET_DEFAULT_SOCKET_BUFFER;
        }
    } else {
        if (use_buffer) {
			if (client->is_waiting == 0)
				client->buffer_size = 0;

            memcpy(client->buffer+client->buffer_size, buf, size);
            client->buffer_size += size;
            real_size = client->buffer_size;
            p = client->buffer;
        } else {
            p = (unsigned char*)buf;
            real_size = size;
        }
        client->is_waiting = 0;
        if (receive_callback != 0) {
            receive_callback(server, client, p, real_size, param);
        }
    }


    return LBS_CODE_OK;
}

lbs_status_t enet_tcp_server_remove_client(
    enet_handle_t* server,
    SOCKET fd
    )
{
    lbs_status_t rc = 0;
    ENET_TCP_SERVER_CLIENT* client, *old_client = 0;
    ENET_TCP_SERVER_CLIENT dup;
    ENET_TCP_SERVER* object;
    cb_tcp_server_disconnected disc_cb;
    void* param;

    /* if the server is closed, we dont need to handle its clients any more */
    rc = enet_handle_wlock(server);
    if (rc != LBS_CODE_OK)
        return rc;
    
    object = enet_get_tcp_server_object(server);
    client = enet_find_client_in_tcp_server(server, fd);
    disc_cb = object->disc_cb;
    param = object->param;
    
    enet_duplicate_server_client_without_data(client, &dup);

    rbt_remove(object->clients_tree, fd, &old_client);
    enet_delete_tcp_server_client_object(client);

    enet_handle_unlock(server);

    if (disc_cb != 0) {
        disc_cb(server, &dup, param);
    }

    return LBS_CODE_OK;
}

/**
 * Process the error code when calling 'recv'
 * @description:
 *     the local connection would be removed if remote connection is down
 * @input: pointer of server handle
 * @input: socket
 * @input: the return code of function 'recv'
 * @return: status
 */
INTERNALCALL lbs_status_t enet_tcp_server_process_recv_error(
    enet_handle_t* server,
    SOCKET fd,
    int sys_err,
    int code
    )
{
    lbs_status_t rc = 0;
    ENET_TCP_SERVER_CLIENT* client;
    ENET_TCP_SERVER* object;
    cb_tcp_server_disconnected disc_cb;
    void* param;

    /* Currently I have found these error codes means disconnection happened
     * from remote client. Maybe more code should be add here
     * If code is zero of 'recv'. Means the remote client called 'closesocket'
     */ 
    if (enet_is_socket_disconnected(sys_err) || code == 0) {

        rc = enet_handle_wlock(server);
        if (rc != LBS_CODE_OK) {
            return rc;
        }

        object = enet_get_tcp_server_object(server);
        //client = enet_find_client_in_tcp_server(server, fd);
        rc = rbt_remove(object->clients_tree, fd, &client);
        if (rc == LBS_CODE_OK) {
            enet_delete_tcp_server_client_object(client);
        }
        disc_cb = object->disc_cb;
        param = object->param;

        enet_handle_unlock(server);

        if (disc_cb != 0) {
            disc_cb(server, client, param);
        }
    } else {
        /* Debug using */
        (void)sys_err;
    }

    return rc;
}

/**
 * The real worker of TCP server
 * @description:
 *     Please check the denination of worker callback function for thread pool
 * @input: task
 * @input: argument
 * @output: output value
 * @inout: customized data for this thread
 * @return: status of this worker
 */
THP_WORKER_STATUS server_worker_thread(void* task, void* arg, void** out, void** customized)
{
    FD_SET readSet;
    FD_SET* jobSet;
    THP_WORKER_STATUS rc = THP_WORKER_OK;
    enet_handle_t* server = (enet_handle_t*)arg;
    lbs_status_t st;
    struct timeval tv = {0, ENET_DEFAULT_SELECT_TIMEOUT};
    int status;
    int timeout;
    unsigned int i = 0;

    /* customized data should be the FD set in this worker */
    if (*customized == 0) {
        *customized = calloc(1, sizeof(FD_SET));
        if (*customized == 0) return THP_WORKER_TERMINATED;
    }
    jobSet = (FD_SET*)*customized;

    /* Process a new task */
    if (task != 0) {
        if (jobSet->fd_count >= FD_SETSIZE) 
            rc = THP_WORKER_TASK_REFUSED;
        else
            FD_SET((SOCKET)task, jobSet);
    }

    /* copy target fd set */
    readSet = *jobSet;

    /* set timeout */
    st = enet_get_tcp_server_attr(server, 0, 0, &timeout, 0, 0, 0, 0, 0);
    if (st != LBS_CODE_OK)
        return THP_WORKER_TERMINATED;
    if (timeout > 0) {
        tv.tv_sec = timeout/1000;
        tv.tv_usec = (timeout%1000)*1000;
    }

    status = select(0, &readSet, 0, 0, &tv);
    if (status == -1)
        return THP_WORKER_IDLE;

    /* Process all the fd to check status */
    for (; i < readSet.fd_count; i++) {
        PACKET_UNIT* packet_unit;
        unsigned size;

        /* Check fd is triggerd */
        if (!FD_ISSET(readSet.fd_array[i], &readSet)) {
            continue;
        }

		/* Allocate packet */
wait_packet:
        packet_unit = enet_alloc_packet_unit();
        if (packet_unit == 0) {
			/* We should wait for packet rather than drop the infomation */
			enet_sleep(10);
			goto wait_packet;
        }

		/* Fetch data from socket */
        status = enet_tcp_socket_recv(readSet.fd_array[i], packet_unit->data,
            ENET_DEFAULT_SOCKET_BUFFER, &size);
        if (status != 0) {

            enet_post_event(server, (void*)status, (void*)readSet.fd_array[i],
                ENET_EVENT_TCP_SERVER_CLIENT_DISCONNECT);

            enet_free_packet_unit(packet_unit);
            FD_CLR(readSet.fd_array[i], jobSet);
            continue;
        }

        if (size > 0) {
            packet_unit->size = size;
            st = enet_post_event(server, packet_unit, (void*)readSet.fd_array[i],
                ENET_EVENT_TCP_SERVER_CLIENT_RECV);
            if (st) {
                enet_free_packet_unit(packet_unit);
            }
        }
    }

    return rc;
}

/**
 * Check if the client is valid for the server
 * @description:
 *     null
 * @input: pointer of the server handle
 * @input: pointer of the client
 * @return:
 *     - ONE means valid
 *     - ZERO means invalid
 */
INTERNALCALL int enet_is_valid_tcp_client(ENET_TCP_SERVER* server, ENET_TCP_SERVER_CLIENT* client)
{
    rbtree_node_t* node;

    if (client == 0)
        return 0;

    node = rbt_search(server->clients_tree, client->fd);
    if (!node || node->data != client) {
        return 0;
    }

    return 1;
}

#include <stdio.h>

unsigned long LBS_THREAD_CALL server_listen_thread(void* param)
{
    enet_handle_t* server = (enet_handle_t*)param;
    ENET_TCP_SERVER* server_obj;
    SOCKET fd;
    FD_SET fdset;
    lbs_status_t st;
    int result;
    struct timeval tm = {0, 500000};

    st = enet_get_tcp_server_socket(server, &fd);
    if (st)
        return st;

    server_obj = enet_get_tcp_server_object(server);
    
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    /* Start working */
    while (server_obj->status != ENET_SERVER_TERMINATE) {
        ENET_TCP_SERVER_CLIENT* client_obj = 0;

        /* Suspending until the request comes */
        result = select(0, &fdset, 0, 0, &tm);
        if (result == 0 || result == -1) {
            continue;
        }

        if (!FD_ISSET(fd, &fdset)) {
            continue;
        }

        st = enet_post_event(server, 0, 0, ENET_EVENT_CONNECT_REQUEST);
        if (st) {
            ;// log error here
        }

		/*???*/
        //enet_sleep(20);
    }

    return 0;
}

INTERNALCALL
lbs_status_t enet_create_tcp_listen_thread(enet_handle_t* server)
{
    ENET_TCP_SERVER* obj = enet_get_tcp_server_object(server);

    obj->listener = CreateThread(0, 0, server_listen_thread, server, CREATE_SUSPENDED, 0);
    if (obj->listener == 0 || obj->listener == INVALID_HANDLE_VALUE) {
        obj->listener = 0;
        return LBS_CODE_WIN_ERROR;
    }

    return LBS_CODE_OK;
}

/**
 * Delete TCP server object
 * @description:
 *     Terminate all threads, thread pool, sockets and client tree.
 * @input: pointer of server object
 * @return: status
 */
INTERNALCALL lbs_status_t enet_delete_tcp_server_object(
    ENET_TCP_SERVER* server_obj
    )
{
    if (server_obj) {
        if (server_obj->listener) {
            TerminateThread(server_obj->listener, 0);
            CloseHandle(server_obj->listener);
        }
        if (server_obj->fd) {
            closesocket(server_obj->fd);
        }
        if (server_obj->clients_tree) {
            rbt_delete_tree(server_obj->clients_tree);
        }
        if (server_obj->worker_pool) {
            thp_delete_thread_pool(server_obj->worker_pool);
        }
        free(server_obj);
    }

    return 0;
}

INTERNALCALL
lbs_status_t enet_new_tcp_server_object(
    int port,
    int flag,
    void* param,
    cb_tcp_server_connected conn_cb,
    cb_tcp_server_received  recv_cb,
    cb_tcp_server_disconnected disc_cb,
    enet_handle_t* handle,
    ENET_TCP_SERVER** server_obj
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* out;
    int thread_count;

    out = (ENET_TCP_SERVER*)calloc(1, sizeof(ENET_TCP_SERVER));
    if (out == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    rc = rbt_new_tree(&out->clients_tree, 0, 0);
    if (rc != LBS_CODE_OK || out->clients_tree == 0) {
        rc = LBS_CODE_NO_MEMORY;
        goto bail_and_cleanup;
    }

    thread_count = (flag & 0x0000ffff)?(flag & 0x0000ffff):10;

    rc = thp_new_thread_pool(150, thread_count, THP_MODE_NORMAL, server_worker_thread, handle, &(out->worker_pool));
    if (rc != 0) {
        goto bail_and_cleanup;
    }

    rc = enet_tcp_server_create_socket(port, out);
    if (rc != 0) {
        goto bail_and_cleanup;
    }

    out->conn_cb = conn_cb;
    out->disc_cb = disc_cb;
    out->recv_cb = recv_cb;
    out->param = param;
    out->connections = 0;
    out->magic = 0x3244A;
    out->flag = flag&0xffff0000;

    *server_obj = out;
    return 0;

bail_and_cleanup:
    enet_delete_tcp_server_object(out);
    return rc;
}


/**
 *      External Functions
 **/
lbs_status_t ENET_CALLCONV enet_new_tcp_server(
    int port,
    int flag,
    int select_timeout,
    void* param,
    cb_tcp_server_connected conn_cb,
    cb_tcp_server_received  recv_cb,
    cb_tcp_server_disconnected disc_cb,
    enet_handle_t** server
    )
{
    lbs_status_t rc;
    enet_handle_t* out;
    ENET_TCP_SERVER* object; // the entity of the handle

    rc = enet_init_event_handler();
    if (rc)
        return rc;

    rc = enet_init_packet_pool();
    if (rc)
        return rc;

    out = enet_new_handle(TRUE);
    if (!out) {
        return LBS_CODE_NO_MEMORY;
    }

    rc = enet_new_tcp_server_object(port, flag, param, conn_cb, recv_cb, disc_cb, out, &object);
    if (rc != 0) {
        enet_delete_handle(out);
        return rc;
    }
    object->select_timeout = select_timeout;

    out->obj = object;
    rc = enet_create_tcp_listen_thread(out);
    if (rc != 0) {
        enet_delete_handle(out);
        enet_delete_tcp_server_object(object);
        return rc;
    }

    *server = out;

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_tcp_server_wait_packet(
    enet_handle_t* server,
    ENET_TCP_SERVER_CLIENT* client,
    void* data,
    int size,
    int is_clean
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* obj;

    if (enet_handle_rlock(server)) {
        return LBS_CODE_INV_HANDLE;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }

    if (!(obj->flag & ENET_SERVER_ATTR_USE_BUFFER)) {
        rc = LBS_CODE_INV_FLAG;
        goto bail;
    }

    if (size + client->buffer_size > ENET_DEFAULT_SOCKET_BUFFER) {
        rc = LBS_CODE_BUFFER_TOO_LARGE;
        goto bail;
    }

    if (0 == enet_is_valid_tcp_client(obj, client)) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }

    if (is_clean) {
        client->buffer_size = 0;
    }

    client->is_waiting = 1;
    rc = 0;

bail:
    enet_handle_unlock(server);

    return rc;
}


lbs_status_t ENET_CALLCONV enet_start_tcp_server(
    enet_handle_t* server
    )
{
    int st;
    lbs_status_t rc;
    ENET_TCP_SERVER* server_obj;
    
    if (enet_handle_rlock(server)) {
        return LBS_CODE_INV_HANDLE;
    }

    server_obj = enet_get_tcp_server_object(server);
    if (server_obj == 0) {
        return LBS_CODE_INV_HANDLE;
    }

    st = listen(server_obj->fd, 0);
    if (st == -1) {
        int err = enet_get_sys_error();
        enet_handle_unlock(server);
        return enet_error_code_convert(err);;
    }

    rc = lbs_resume_thread(server_obj->listener);
    if (rc != LBS_CODE_OK) {
        enet_handle_unlock(server);
        return rc;
    }

    rc = thp_start_thread_pool(server_obj->worker_pool);
    if (rc != 0) {
        lbs_suspend_thread(server_obj->listener);
        enet_handle_unlock(server);
        return rc;
    }

    enet_handle_unlock(server);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_stop_tcp_server(
    enet_handle_t* server
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* obj;

    if (enet_handle_wlock(server)) {
        return LBS_CODE_INV_HANDLE;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        enet_handle_unlock(server);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    rc = lbs_suspend_thread(obj->listener);
    if (rc != LBS_CODE_OK) {
        enet_handle_unlock(server);
        return rc;
    }

    rc = thp_stop_thread_pool(obj->worker_pool);
    if (rc != LBS_CODE_OK) {
        enet_handle_unlock(server);
        return rc;
    }

    enet_handle_unlock(server);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_delete_tcp_server(
    enet_handle_t* server
    )
{
    ENET_TCP_SERVER* obj;

    if (enet_handle_wlock(server)) {
        return LBS_CODE_INV_HANDLE;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        enet_handle_unlock(server);
        return LBS_CODE_HANDLE_TYPE_MISMATCH;
    }

    enet_delete_tcp_server_object(obj);

    server->obj = 0;

    enet_handle_unlock(server);
    enet_delete_handle(server);

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_disconnect_client_from_tcp_server(
    enet_handle_t* server,
    ENET_TCP_SERVER_CLIENT* client
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* obj;

    if (client == 0) 
        return LBS_CODE_INV_PARAM;
    
    rc = enet_handle_wlock(server);
    if (rc != LBS_CODE_OK) {
        return rc;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }

    if (0 == enet_is_valid_tcp_client(obj, client)) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }

    /* When we close the socket. This fd should be invalid and the worker would
     * know. Then worker should remove it from FD_SET and clean the memory
     */
    closesocket(client->fd);
    
    rc = LBS_CODE_OK;
bail:
    enet_handle_unlock(server);

    return rc;
}

lbs_status_t ENET_CALLCONV enet_tcp_server_send_to_client(
    enet_handle_t* server,
    ENET_TCP_SERVER_CLIENT* client,
    void* data,
    int size
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* obj;
    int sended = 0;
    int use_packet;
    
    if (data == 0 || size == 0)
        return LBS_CODE_INV_PARAM;
    
    rc = enet_handle_rlock(server);
    if (rc != LBS_CODE_OK) {
        return rc;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }
    
    use_packet = obj->flag & ENET_SERVER_ATTR_USE_PACKET;

    if (size > ENET_DEFAULT_SOCKET_BUFFER && use_packet) {
        rc = LBS_CODE_BUFFER_TOO_LARGE;
        goto bail;
    }

    if (0 == enet_is_valid_tcp_client(obj, client)) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }
    
    enet_handle_unlock(server);

    rc = enet_tcp_socket_send(client->fd, data, size, use_packet);

    return rc;

bail:
    enet_handle_unlock(server);

    return rc;
}

lbs_status_t ENET_CALLCONV enet_tcp_server_get_client_count(
    enet_handle_t* server,
    int* count
    )
{
    lbs_status_t rc;
    ENET_TCP_SERVER* obj;
    int sended = 0;
    
    if (count == 0)
        return LBS_CODE_INV_PARAM;
    
    rc = enet_handle_rlock(server);
    if (rc != LBS_CODE_OK) {
        return rc;
    }

    obj = enet_get_tcp_server_object(server);
    if (obj == 0) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }

    *count = obj->clients_tree->count;
    
    rc = LBS_CODE_OK;
bail:
    enet_handle_unlock(server);

    return rc;
}

