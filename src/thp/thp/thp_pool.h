#ifndef _THP_INSTANCE_H_
#define _THP_INSTANCE_H_


#include "thp_types.h"

lbs_status_t thp_new_pool(
    int pressure_limit,
    int threads, 
    int mode,
    WORKER_FUNC real_worker,
    void* param,
    thp_handle_t** pool
    );

lbs_status_t thp_delete_pool(
    thp_handle_t* pool
    );

THP_POOL* thp_get_pool_object(thp_handle_t* pool);
thp_handle_t* thp_get_task_server_from_pool(thp_handle_t* pool);
thp_handle_t* thp_get_thread_watcher_from_pool(thp_handle_t* pool);

#endif
