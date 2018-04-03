#ifndef _LBS_THREADPOOL_H_
#define _LBS_THREADPOOL_H_

#define WORKERCALL _stdcall

/*
 * This is the handle value of thread pool object. It is a
 * uinueqe id for each object. Be carefull that now we do
 * not record it in list. So if caller free this object NEVER
 * use it again.
 */
typedef unsigned long lbs_thread_pool_handle_t;

/*
 * This is the primary form of woker call function. Caller
 * should prepare a function like that and provide its address
 * to thread pool.
 * uniq_data:
 *   This parameter is used to save some data that caller want
 *   to use in every loop of calling worker func. User should
 *   allocate memory for it in the first run of worker func. 
 *   The same data will be passed in during the other callings.
 */
typedef int (WORKERCALL * workercall)(
    void* param, // the parameter when creating thread pool
    void* task, // the pointer of task
    unsigned char** uniq_data // See function description
    );

/*
 * Flag of thread termination. If worker receives a task which
 * value is this flag it should do finalizations.
 */
#define LBS_THREADPOOL_TERMINATION_FLAG 0x80003244

/*
 * Flags of running method
 */
#define LBS_THREADPOOL_NORMAL   0x00000001 // Thread will be blocked if there is no task
#define LBS_THREADPOOL_CONTINUE 0x00000002 // Thread will continue run even if there is no task
#define LBS_THREADPOOL_AUTOFREE 0x00000010 // Free task autometicly
#define LBS_THREADPOOL_REUSE    0x00000020 // Reuse task if failed.

/*
 * Return value of worker func.
 * !! NOTE:
 *         Caller must make sure the worker func can return these
 *         values in different cases.
 */
#define LBS_THREADPOOL_WORKER_OK 0   // Task has been excuted
#define LBS_THREADPOOL_WORKER_ERR 1  // Task has failed
#define LBS_TRHEADPOOL_WORKER_DENY 2 // Worker cannot stop now
#define LBS_THREADPOOL_WORKER_CORRUPT 3 // Data has corrupted. Terminate thread

#define LBS_THREADPOOL_ECODE_BASE 3000

typedef enum _lbs_threadpool_erron {
    LBS_THREADPOOL_OK = 0,
    LBS_THREADPOOL_INV_PARAM = 1 + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_NO_MEM = 2 + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_NO_RSC = 3 + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_INV_HANDLE = 4 + LBS_THREADPOOL_ECODE_BASE,
    /*
    LBS_THREADPOOL_ =  + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_ =  + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_ =  + LBS_THREADPOOL_ECODE_BASE,
    LBS_THREADPOOL_ =  + LBS_THREADPOOL_ECODE_BASE,
    */
    LBS_THREADPOOL_ERR_CNT = 0xffffffff,
}lbs_threadpool_status_t;


#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Create thread pool. Max min threads count
     */
    lbs_threadpool_status_t lbs_create_threadpool(
        lbs_thread_pool_handle_t* handle, // [OUT]Handle of thread pool
        int max_threads, // Max threads count
        int min_threads, // Min threads count
        int flag, // Running method flag
        workercall realworker, // Pointer of worker func
        void* param // This one will be passed to worker call back
        );
    
    /**
     * Push a new task into message queue
     * Context of task will be passed to worker func.
     * Caller can free this copy manually or let thread pool
     * free is autometicly by using flag AUTOFREE.
     */
    lbs_threadpool_status_t lbs_threadpool_push_task(
        lbs_thread_pool_handle_t handle,
        void* context,
        int size
        );

    /**
     * Set thread pool time limitations.
     */
    lbs_threadpool_status_t lbs_threadpool_set_time_limit(
        int thread_termination_time,
        int kickoff_time
        );

    /**
     * Get current thread count.
     */
    int lbs_threadpool_get_thread_count(
        lbs_thread_pool_handle_t handle
        );

    /**
     * Get current task count.
     */
    int lbs_threadpool_get_task_count(
        lbs_thread_pool_handle_t handle
        );

    /**
     * Destroy thread pool
     */
    lbs_threadpool_status_t lbs_destroy_threadpool(
        lbs_thread_pool_handle_t* handle
        );

#ifdef __cplusplus
};
#endif

#endif //_LBS_THREADPOOL_H_


