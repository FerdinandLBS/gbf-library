#ifndef _THP_H_
#define _THP_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include <lbs_errno.h>

typedef enum __thp_worker_status{
    /* the worker works fine */
    THP_WORKER_OK,

    /* the task is refused by the worker */
    THP_WORKER_TASK_REFUSED,

    /* the worker should be terminated due to some reason */
    THP_WORKER_TERMINATED,

    /* the worker is idle. Sleep 10 ms to free cpu */
    THP_WORKER_IDLE,
}THP_WORKER_STATUS;

typedef struct __thp_handle thp_handle_t;

typedef THP_WORKER_STATUS (*WORKER_FUNC)(void* task, void* arg, void** out, void** customized);
typedef int (*ERROR_FUNC)(lbs_status_t code);

/* Common container of any objects in this library */
struct __thp_handle {
    void* lock;
    void* obj; // The obj of object
    int alive;
    int refcnt; // atom operations are necessary
    thp_handle_t* next;
    thp_handle_t* prev;
};

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


/*
 * Thread pool running mode
 * For NORMAL mode:
 *   TCP server, kinds of monitor
 * For caller mode:
 *   data processor
 */
#define THP_MODE_NORMAL         0 // The real worker will be called everytime even though there is no task
#define THP_MODE_CALL_WITH_TASK 1 // The real worker will be called when there is a task

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Create new thread pool
     * @description:
     *     null
     * @input: pressure limitaion. in 100 percent. eg. 160 means when Task queue
     *         is larger than 1.6 times of threads count create new thread.
     * @input: initialized thread counts
     * @input: running mode
     * @input: the real worker callback function
     * @input: the parameter for real worker
     * @output: the pointer of thread pool
     * @return: status
     */
    lbs_status_t thp_new_thread_pool(
        int pressure_limit,
        int threads, 
        int mode,
        WORKER_FUNC real_worker,
        void* param,
        thp_handle_t** pool
        );

    lbs_status_t thp_start_thread_pool(
        thp_handle_t* pool
        );

    lbs_status_t thp_stop_thread_pool(
        thp_handle_t* pool
        );

    lbs_status_t thp_delete_thread_pool(
        thp_handle_t* pool
        );

    lbs_status_t thp_deliver_task(
        thp_handle_t* pool,
        void* task,
        unsigned int size,
        void* output
        );
    
    THP_THREAD_INFO* thp_get_threads(
        thp_handle_t* pool, 
        THP_THREAD_INFO* prev
        );

    lbs_status_t thp_get_pending_task_count(
        thp_handle_t* pool,
        int* count
        );

    lbs_status_t thp_get_running_thread_count(
        thp_handle_t* pool,
        int* count
        );

#ifdef __cplusplus
};
#endif

#endif
