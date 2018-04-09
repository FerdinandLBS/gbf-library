#include "thp_pool.h"
#include "thp_handle.h"
#include "thp_task.h"
#include "thp_thread.h"

/*
 * Using link table to maintain all instances.
 * We suppose that there won't too much instances and we will also
 * have a limitation for obj counts
 */
#define THP_MAX_INSTANCE_COUNT 100

/*
 * ---- Local functions ----
 */
INTERNALCALL
void thp_pool_object_cleanup(THP_POOL* pool)
{
    if (pool == 0)
        return;

    if (pool->task_server) {
        thp_delete_task_server(pool->task_server);
    }

    if (pool->thread_watcher) {
        thp_delete_watcher(pool->thread_watcher);
    }

    free(pool);
}

INTERNALCALL
THP_POOL* thp_new_pool_object(int mode)
{
    THP_POOL* out = (THP_POOL*)calloc(1, sizeof(THP_POOL));
    if (!out)
        return 0;

    out->mode = mode;
    return out;
}

lbs_status_t thp_new_pool(
    int pressure_limit,
    int threads, 
    int mode,
    WORKER_FUNC real_worker,
    void* param,
    thp_handle_t** pool
    )
{
    lbs_status_t rc = LBS_CODE_OK;
    thp_handle_t* out;

    out = thp_new_handle(1);
    rc = LBS_CODE_NO_MEMORY;
    if (out == 0)
        goto bail_and_cleanup; 

    out->obj = thp_new_pool_object(mode);
    if (out->obj == 0)
        goto bail_and_cleanup; 

    rc = thp_new_task_server(out, &(((THP_POOL*)out->obj)->task_server));
    if (rc != LBS_CODE_OK)
        goto bail_and_cleanup;

    rc = thp_new_watcher(out, pressure_limit, threads, real_worker, param, &(((THP_POOL*)out->obj)->thread_watcher));
    if (rc != LBS_CODE_OK)
        goto bail_and_cleanup;

    *pool = out;

    return LBS_CODE_OK;

bail_and_cleanup:
    if (out) {
        thp_pool_object_cleanup(out->obj);
        thp_del_handle(out);
    }
    return rc;
}

lbs_status_t thp_delete_pool(thp_handle_t* pool)
{
    lbs_status_t rc;

    rc = thp_handle_wlock(pool);
    if (rc != 0)
        return rc;
    
    thp_pool_object_cleanup(pool->obj);
    pool->alive = 0;

    thp_handle_unlock(pool);

    return LBS_CODE_OK;
}

THP_POOL* thp_get_pool_object(thp_handle_t* pool)
{
    return (THP_POOL*)(pool->obj);
}

thp_handle_t* thp_get_task_server_from_pool(thp_handle_t* pool)
{
    THP_POOL* pool_obj = thp_get_pool_object(pool);
    THP_TASK_SERVER* out = thp_get_task_server_object(pool_obj->task_server);

    return pool_obj->task_server;
}

thp_handle_t* thp_get_thread_watcher_from_pool(thp_handle_t* pool)
{
    return (((THP_POOL*)pool->obj)->thread_watcher);
}
