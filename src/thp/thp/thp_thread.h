#ifndef _THP_THREAD_H_
#define _THP_THREAD_H_

#include "thp_rwlock.h"
#include "thp_types.h"

#define THP_DEFAULT_THREAD_COUNT 8
#define THP_WATCHER_HEART_BASE 500 // Milli-second. Run maximum to 135 years at 250ms

#define THP_WATCHER_NULL  0
#define THP_WATCHER_READY 1
#define THP_WATCHER_RUN   2
#define THP_WATCHER_PUASE 3
#define THP_WATCHER_TERM  4
#define THP_WATCHER_ERROR 5

lbs_status_t thp_new_watcher(
    thp_handle_t* parent,
    int pressure_limit,
    int threads, 
    WORKER_FUNC worker,
    void* param,
    thp_handle_t** watcher
    );

lbs_status_t thp_start_threads_and_watcher(thp_handle_t* watcher);
lbs_status_t thp_stop_threads_and_watcher(thp_handle_t* watcher);
lbs_status_t thp_puase_threads_and_watcher(thp_handle_t* watcher);
lbs_status_t thp_resume_threads_and_watcher(thp_handle_t* watcher);
lbs_status_t thp_delete_watcher(thp_handle_t* watcher);

THP_THREAD_WATCHER* thp_get_watcher_object(thp_handle_t* watcher);
THP_THREAD_INFO* thp_get_first_thread_in_watcher(thp_handle_t* watcher);
THP_THREAD_INFO* thp_find_thread_in_watcher(thp_handle_t* watcher, THP_THREAD_INFO* target);

#endif
