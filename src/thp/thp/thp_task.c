#include "stdlib.h"
#include "thp_handle.h"
#include "thp_task.h"

/* Task Management
 *
 * Task is designed as a FIFO queue.
 * It is threads safe.
 * Task is not handled immediately. Invoker will get success code
 * if task is push successfully.
 */

/*
 * ----- Local Functions -----
 */
INTERNALCALL
void thp_clean_package_chain(
    THP_TASK_PACKAGE* head
    )
{
    THP_TASK_PACKAGE* node;

    if (!head)
        return;

    node = head;
    while (node) {
        THP_TASK_PACKAGE* temp = node->next;
        free(node);
        node = temp;
    }
}

INTERNALCALL
int thp_package_push_tail(
    THP_TASK_PACKAGE** head,
    THP_TASK_PACKAGE* package
    )
{
    int i = 1;
    THP_TASK_PACKAGE* node = *head;

    if (*head == 0) {
        *head = package;
        return i;
    }

    i++;
    while (node->next) {node = node->next; i++;}
    node->next = package;
    package->next = 0;

    return i;
}

INTERNALCALL
THP_TASK_SERVER* thp_new_task_server_object(thp_handle_t* parent)
{
    THP_TASK_SERVER* out = (THP_TASK_SERVER*)calloc(1, sizeof(THP_TASK_SERVER));

    if (!out)
        return 0;

    out->parent = parent;

    return out;
}


INTERNALCALL
void thp_remove_task_server_object(thp_handle_t* server)
{
    free(server->obj);
    server->alive = 0;
}

INTERNALCALL
void thp_clean_task_package(THP_TASK_PACKAGE* package)
{
    package->next = 0;
    package->output = 0;
    package->size = 0;
    package->task = 0;
}

THP_TASK_SERVER* thp_get_task_server_object(thp_handle_t* server)
{
    return (THP_TASK_SERVER*)(server->obj);
}

/* Create task server */
lbs_status_t thp_new_task_server(
    thp_handle_t* parent,
    thp_handle_t** task_server
    )
{
    THP_TASK_SERVER* server_obj;
    thp_handle_t* out = thp_new_handle(1);
    if (!out)
        return LBS_CODE_NO_MEMORY;

    server_obj = thp_new_task_server_object(parent);
    if (!server_obj) {
        thp_del_handle(out);
        return LBS_CODE_NO_MEMORY;
    }

    out->obj = (void*)server_obj;
    server_obj->parent = parent;

    *task_server = out;

    return LBS_CODE_OK;
}

/* Release and clean task server */
lbs_status_t thp_delete_task_server(
    thp_handle_t* task_server
    )
{
    if (task_server) {
        lbs_status_t rc;
        THP_TASK_SERVER* server;

        rc = thp_handle_wlock(task_server); 
        if (rc != 0) {
            return rc;
        }

        server = thp_get_task_server_object(task_server);
        thp_clean_package_chain(server->packages);
        thp_remove_task_server_object(task_server);

        thp_handle_unlock(task_server);
        return LBS_CODE_OK;
    } else {
        return LBS_CODE_INV_HANDLE;
    }
}

/* Prepare a task pachage */
lbs_status_t thp_prepare_task_package(
    thp_handle_t* task_server,
    void* real_task,
    unsigned int size,
    void* output,
    THP_TASK_PACKAGE** package
    )
{
    lbs_status_t rc;
    THP_TASK_PACKAGE* out;
    THP_TASK_SERVER* object;

    rc = thp_handle_wlock(task_server);
    if (rc != 0) {
        return rc;
    }

    object = thp_get_task_server_object(task_server);
    if (object->package_pool == 0) {
        out = (THP_TASK_PACKAGE*)malloc(sizeof(THP_TASK_PACKAGE));
        if (!out) {
            thp_handle_unlock(task_server);
            return LBS_CODE_NO_MEMORY;
        }
    } else {
        out = object->package_pool;
        object->package_pool = out->next;
    }

    thp_handle_unlock(task_server);

    out->next = 0;
    out->output = output;
    out->task = real_task;
    out->size = size;

    *package = out;

    return LBS_CODE_OK;
}

/* Push the task to the tail */
lbs_status_t thp_push_task(
    thp_handle_t* task_server, 
    THP_TASK_PACKAGE* package
    )
{
    lbs_status_t rc;
    int i;
    THP_TASK_SERVER* server;

    rc = thp_handle_wlock(task_server);
    if (rc != LBS_CODE_OK) 
        return rc;

    server = thp_get_task_server_object(task_server);
    i = thp_package_push_tail(&(server->packages), package);
    server->task_count++;

    thp_handle_unlock(task_server);

    return LBS_CODE_OK;
}

/* Pop the first task a the queue */
lbs_status_t thp_pop_task(
    thp_handle_t* task_server,
    THP_TASK_PACKAGE** package
    )
{
    lbs_status_t rc;
    THP_TASK_SERVER* server;
    THP_TASK_PACKAGE* p;

    rc = thp_handle_wlock(task_server);
    if (rc != LBS_CODE_OK) {
        return rc;
    }

    server = thp_get_task_server_object(task_server);
    p = server->packages;
    if (p) {
        server->packages = p->next;
        server->task_count--;
        p->next = 0;
        if (server->task_count<0)
            server->task_count = 0;
    }

    thp_handle_unlock(task_server);

    *package = p;
    if (p)
        return LBS_CODE_OK;
    else
        return LBS_CODE_NO_TASK;
}

/* Get current task counts */
lbs_status_t thp_get_task_count(
    thp_handle_t* task_server,
    int* count
    )
{
    lbs_status_t rc;
    THP_TASK_SERVER* server;

    rc = thp_handle_rlock(task_server);
    if (rc != LBS_CODE_OK)
        return rc;

    server = thp_get_task_server_object(task_server);
    *count = server->task_count;

    thp_handle_unlock(task_server);

    return 0;
}

/* Clean a task package */
void* thp_delete_task_package(
    THP_TASK_PACKAGE* package
    )
{
    void* out = 0;

    if (package) {
        out = package->output;
        free(package);
    }
    return out;
}

void thp_recollect_task_package(
    thp_handle_t* task_server,
    THP_TASK_PACKAGE** package
    )
{
    lbs_status_t rc;
    THP_TASK_PACKAGE* node;
    THP_TASK_SERVER* object = thp_get_task_server_object(task_server);

    rc = thp_handle_wlock(task_server);
    if (rc != 0) {
        free(*package);
        *package = 0;
        return;
    }

    node = object->package_pool;
    if (node == 0) {
        object->package_pool = *package;
    } else {
        while (node->next) node = node->next;
        node->next = *package;
        thp_clean_task_package(*package);
        *package = 0;
    }

    thp_handle_unlock(task_server);
}
