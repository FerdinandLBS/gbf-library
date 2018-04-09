#ifndef _THP_TYPES_H_
#define _THP_TYPES_H_

#ifdef _WIN32
#include <Windows.h>
#endif
#include <lbs_errno.h>
#include "thp_rwlock.h"

typedef unsigned int lbs_status_t;
typedef struct __thp_handle thp_handle_t;

typedef enum __thp_worker_status{
    THP_WORKER_OK,
    THP_WORKER_TASK_REFUSED,
    THP_WORKER_TERMINATED,

    /* the worker is idle. Sleep 10 ms to free cpu */
    THP_WORKER_IDLE,
}THP_WORKER_STATUS;

typedef THP_WORKER_STATUS (*WORKER_FUNC)(void* task, void* arg, void** out, void** customized);
typedef int (*ERROR_FUNC)(lbs_status_t code);

/* Error log:
 *
 * Record errors
 */
typedef struct __thp_error_log {
    unsigned int time;
    lbs_status_t code;
    char location[32];

    struct __thp_error_log* next;
}THP_ERROR_LOG;

/* Task package:
 *
 * Push, pop task.
 * Ouput data if caller needs
 */
typedef struct __thp_task_package{
    void* task;
    unsigned int size;
    void* output;
    struct __thp_task_package* next;
}THP_TASK_PACKAGE;

/* Task server:
 *
 * Push, pop taskes. Handle locks inside funtions
 */
typedef struct __thp_task_server{
    thp_handle_t* parent;
    unsigned int task_count;
    THP_TASK_PACKAGE* package_pool;
    THP_TASK_PACKAGE* packages;
}THP_TASK_SERVER;


/* Thread Info
 * 
 * Contain thread information. Can struct as chain for upper-layer management
 */
typedef struct __thp_thread_info{
    unsigned int id;
#ifdef _WIN32
    HANDLE handle;
#else
    int handle;
#endif
    unsigned int heart;
    unsigned int load;

    struct __thp_thread_info* next;
}THP_THREAD_INFO;

/* Manage threads:
 *
 * Create threads pool.
 * maintain pool capacity.
 * Watch threads' states. 
 * Terminate threads.
 */
typedef struct __thp_thread_watcher{
    thp_handle_t* parent; // the parent handle. Means the pool
    unsigned int status; // the status of watcher
    unsigned int counts; // the threads counts
    int pressure_limit; // the pressure limitation. in 100 percent

    void* param; // The parameter passed by caller for real worker
    WORKER_FUNC real_worker; // address of real worker

    THP_THREAD_INFO* watcher; // the watcher thread information
    THP_THREAD_INFO* threads; // the worker threads information
}THP_THREAD_WATCHER;

/* Threads pool Instance */
typedef struct __thp_instance {
    int mode;
    thp_handle_t* thread_watcher; // link with a watcher
    thp_handle_t* task_server; // link with a task server
}THP_POOL;

/* Common container of any objects in this library */
struct __thp_handle {
    void* lock; // read/write lock
    void* obj; // The obj of object
    int alive; // if the entity is killed
    int refcnt; // atom operations are necessary
    thp_handle_t* next; 
    thp_handle_t* prev;
};

/*
 * Thread pool running mode
 */
#define THP_MODE_NORMAL 0
#define THP_MODE_CALL_WITH_TASK 1


#ifndef INTERNALCALL
#define INTERNALCALL static
#endif

#endif
