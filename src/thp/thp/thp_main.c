#include "thp_pool.h"
#include "thp_handle.h"
#include "thp_types.h"
#include "thp_log.h"
#include "thp_task.h"
#include "thp_thread.h"

lbs_status_t thp_new_thread_pool(
    int pressure_limit,
    int threads,
    int mode,
    WORKER_FUNC real_worker,
    void* param,
    thp_handle_t** pool
    )
{
    thp_handle_t* out;
    lbs_status_t rc = LBS_CODE_OK;

    if (pool == 0 || real_worker == 0)
        return LBS_CODE_INV_PARAM;

    rc = thp_new_pool(pressure_limit, threads, mode, real_worker, param, &out);
    if (rc != LBS_CODE_OK)
        return rc;

    *pool = out;
    
    return rc;
}

lbs_status_t thp_start_thread_pool(
    thp_handle_t* pool
    )
{
    if (pool == 0)
        return LBS_CODE_INV_PARAM;

    return thp_start_threads_and_watcher(thp_get_thread_watcher_from_pool(pool));
}

lbs_status_t thp_stop_thread_pool(
    thp_handle_t* pool
    )
{
    if (pool == 0)
        return LBS_CODE_INV_PARAM;

    return thp_stop_threads_and_watcher(thp_get_thread_watcher_from_pool(pool));
}

lbs_status_t thp_delete_thread_pool(
    thp_handle_t* pool
    )
{
    if (pool == 0)
        return 0;

    thp_stop_threads_and_watcher(thp_get_thread_watcher_from_pool(pool));
    thp_delete_pool(pool);

    return 0;
}

lbs_status_t thp_deliver_task(
    thp_handle_t* pool,
    void* task,
    unsigned int size,
    void* output
    )
{
    lbs_status_t rc;
    THP_TASK_PACKAGE* package;
    thp_handle_t* task_server;

    if (pool == 0 || task == 0)
        return LBS_CODE_INV_PARAM;

    rc = thp_handle_rlock(pool);
    if (rc != 0) {
        return rc;
    }

    task_server = thp_get_task_server_from_pool(pool);

    rc = thp_prepare_task_package(task_server, task, size, output, &package);
    if (rc) {
        thp_handle_unlock(pool);
        return rc;
    }

    rc = thp_push_task(task_server, package);
    if (rc) {
        thp_recollect_task_package(task_server, &package);
        thp_handle_unlock(pool);
        return rc;
    }

    thp_handle_unlock(pool);
    return LBS_CODE_OK;
}

THP_THREAD_INFO* thp_get_threads(thp_handle_t* pool, THP_THREAD_INFO* prev)
{
    THP_POOL* object;
    THP_THREAD_INFO* out = prev;
    lbs_status_t rc;

    if (pool == 0)
        return 0;

    object = thp_get_pool_object(pool);

    rc = thp_handle_rlock(pool);
    if (rc != 0)
        return 0;

    if (prev == 0) {
        out = thp_get_first_thread_in_watcher(object->thread_watcher);
    } else {
        out = thp_find_thread_in_watcher(object->thread_watcher, prev);
        if (out) {
            out = out->next;
        }
    }
    thp_handle_unlock(pool);

    return out;
}

lbs_status_t thp_get_pending_task_count(
    thp_handle_t* pool,
    int* count
    )
{
    lbs_status_t rc;
    int out_cnt = 0;
    THP_POOL* object;
    THP_TASK_SERVER* task_server;

    if (pool == 0 || count == 0) {
        return LBS_CODE_INV_PARAM;
    }

    object = thp_get_pool_object(pool);

    rc = thp_handle_rlock(pool);
    if (rc != 0) {
        return rc;
    }

    rc = thp_handle_rlock(object->task_server);
    if (rc != 0)
        return rc;

    task_server = thp_get_task_server_object(object->task_server);
    out_cnt = task_server->task_count;

    thp_handle_unlock(object->task_server);

    rc = thp_handle_unlock(pool);
    if (rc != 0) {
        return rc;
    }

    *count = out_cnt;

    return LBS_CODE_OK;
}

lbs_status_t thp_get_running_thread_count(
    thp_handle_t* pool,
    int* count
    )
{
    int out_cnt = 0;
    lbs_status_t rc;
    THP_POOL* object;
    THP_THREAD_WATCHER* watcher;

    if (pool == 0 || count == 0) {
        return LBS_CODE_INV_PARAM;
    }

    object = thp_get_pool_object(pool);
    rc = thp_handle_rlock(pool);
    if (rc != 0) {
        return rc;
    }

    watcher = thp_get_watcher_object(object->thread_watcher);
    out_cnt = watcher->counts;

    rc = thp_handle_unlock(pool);
    if (rc != 0) {
        return rc;
    }

    *count = out_cnt;

    return LBS_CODE_OK;
}

