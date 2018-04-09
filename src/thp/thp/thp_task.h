#ifndef _THP_TASK_H_
#define _THP_TASK_H_

#include "thp_types.h"

lbs_status_t thp_new_task_server(
    thp_handle_t* parent,
    thp_handle_t** task_server
    );

lbs_status_t thp_delete_task_server(
    thp_handle_t* task_server
    );

lbs_status_t thp_prepare_task_package(
    thp_handle_t* task_server,
    void* real_task,
    unsigned int size,
    void* output,
    THP_TASK_PACKAGE** package
    );

lbs_status_t thp_push_task(
    thp_handle_t* task_server, 
    THP_TASK_PACKAGE* package
    );

lbs_status_t thp_pop_task(
    thp_handle_t* task_server,
    THP_TASK_PACKAGE** package
    );

lbs_status_t thp_get_task_count(
    thp_handle_t* task_server,
    int* count
    );

void* thp_delete_task_package(
    THP_TASK_PACKAGE* package
    );

THP_TASK_SERVER* thp_get_task_server_object(
    thp_handle_t* server
    );

void thp_recollect_task_package(
    thp_handle_t* task_server,
    THP_TASK_PACKAGE** package
    );

#endif
