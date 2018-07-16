#include <Windows.h>
#include "thp_pool.h"
#include "thp_handle.h"
#include "thp_thread.h"
#include "thp_task.h"

/* Flags when create thread */
typedef enum {
    RUN_IMMEDIATELY,
    SUSPEND,
}CREATE_THREAD_FLAG;

/*
 * ----- Local Functions -----
 */
INTERNALCALL
lbs_status_t thp_suspend_thread(THP_THREAD_INFO* thread)
{
    int rc;

    if (thread == 0 || thread->handle == 0) 
        return LBS_CODE_INV_PARAM;

    rc = SuspendThread(thread->handle);
    if (rc != 1 && rc > 0) {
        return LBS_CODE_SUSPEND_BY_OTHERS;
    }
    if (rc == -1) {
        rc = GetLastError();
        return rc;
    }

    return LBS_CODE_OK;
}

INTERNALCALL
lbs_status_t thp_resume_thread(THP_THREAD_INFO* thread)
{
    unsigned int rc;

    if (thread == 0 || thread->handle == 0) 
        return LBS_CODE_INV_PARAM;

    rc = ResumeThread(thread->handle);
    if (rc != 1) {
        /* There should be no way to get a '0' from this function */
        if (rc != (unsigned int)-1)
            return LBS_CODE_SUSPEND_BY_OTHERS;

        rc = GetLastError();
        switch (rc) {
        default:
            rc = rc;
        }
        return rc;
    }

    return LBS_CODE_OK;
}

THP_THREAD_WATCHER* thp_get_watcher_object(thp_handle_t* watcher)
{
    return (THP_THREAD_WATCHER*)(watcher->obj);
}

INTERNALCALL
thp_handle_t* thp_get_linked_task_server_with_watcher(thp_handle_t* watcher)
{
    THP_THREAD_WATCHER* watcher_obj = thp_get_watcher_object(watcher);
    thp_handle_t* task_server = thp_get_task_server_from_pool(watcher_obj->parent);
    return task_server;
}

/*
 * Find a the thread info for current thread
 *
 * Layer: 2
 * Locking: yes
 */
THP_THREAD_INFO* thp_find_my_thread_info(thp_handle_t* watcher)
{
    lbs_status_t rc;
    unsigned int id;
    THP_THREAD_INFO* node;
    THP_THREAD_WATCHER* object;

    if (watcher == 0)
        return 0;

    id = GetCurrentThreadId();

    rc = thp_handle_rlock(watcher);
    if (rc != 0) {
        return 0;
    }

    object = thp_get_watcher_object(watcher);
    node = object->threads;
    while (node) {
        if (node->id == id)
            break;

        node = node->next;
    }

    thp_handle_unlock(watcher);

    return node;
}

/*
 * Agent worker in thread pool
 *
 * Layer: 2
 * Locking: no
 * 
 * Operate tasks and callback to real worker
 */
unsigned int WINAPI worker_thread(void* param)
{
    thp_handle_t* watcher = (thp_handle_t*)param;
    THP_THREAD_WATCHER* watcher_object = thp_get_watcher_object(watcher);
    THP_THREAD_INFO* myinfo = thp_find_my_thread_info(watcher);
    thp_handle_t* task_server = thp_get_linked_task_server_with_watcher(watcher);
    THP_TASK_PACKAGE* package = 0;
    THP_WORKER_STATUS rc;
    int mode = thp_get_pool_object(watcher_object->parent)->mode;
    void* task;
    void* output;
    void* customized = 0;

    while (1) {
        /* This signal is a shared resource. It can be change only in invoker functions */
        if (watcher_object->status == THP_WATCHER_TERM) 
            break;

        /* Every single thread should have a heart beat so that invoker will not
         * block the real worker too long. But this behavior may cause memory leak.
         */
        if (myinfo->heart < watcher_object->watcher->heart) {
            myinfo->heart = watcher_object->watcher->heart;
            //myinfo->load = 0;
        }

        /* Get one task from the queue */
        if (LBS_CODE_OK == thp_pop_task(task_server, &package)) {
            task = package->task;
            output = package->output;
            myinfo->load++;
        } else {
            task = output = 0;
        }

        if (mode == THP_MODE_CALL_WITH_TASK && task)
            rc = watcher_object->real_worker(task, watcher_object->param, output, &customized);
        if (mode == THP_MODE_NORMAL)
            rc = watcher_object->real_worker(task, watcher_object->param, output, &customized);

        switch (rc) {
        case THP_WORKER_OK:
            break;
        case THP_WORKER_TASK_REFUSED:
            package->task = task;
            thp_push_task(task_server, package);
            
            package = 0; /* Prevent the reuse of package */
            break;
        case THP_WORKER_TERMINATED:
            /* Not tested, the real worker should release customized data itself */
            return 0; 
            break;
        case THP_WORKER_IDLE:
            Sleep(10);
            break;
        default:
            ;
        }

        if (package)
            thp_recollect_task_package(task_server, &package);

        SwitchToThread();

        /* Reset the worker status */
        rc = THP_WORKER_OK;
    }

    return 0;
}

/**
 * Create one thread and append its information to watcher
 *
 * Layer: 3
 * Locking-unlocking: yes
 *
 */
lbs_status_t thp_create_worker_thread(
    thp_handle_t* watcher,
    CREATE_THREAD_FLAG flag,
    THP_THREAD_INFO** new_info
    )
{
    THP_THREAD_WATCHER* object;
    THP_THREAD_INFO* out;
    THP_THREAD_INFO* node;
    lbs_status_t rc;

    out = (THP_THREAD_INFO*)calloc(1, sizeof(THP_THREAD_INFO));
    if (!out) {
        return LBS_CODE_NO_MEMORY;
    }

    out->heart = 0;
    out->next = 0;

    out->handle = CreateThread(0, 0, worker_thread, watcher, CREATE_SUSPENDED, &out->id);
    if (out->handle == INVALID_HANDLE_VALUE) {
        free(out);
        return 0;
    }
    
    /* Get the object of watcher. Here we need readable access only */
    rc = thp_handle_wlock(watcher);
    if (rc != LBS_CODE_OK) {
        TerminateThread(out->handle, 0);
        CloseHandle(out->handle);
        free(out);
        return rc;
    }

    object = thp_get_watcher_object(watcher);

    /* Do insert */
    node = object->threads;
    if (!node) {
        object->threads = out;
    } else {
        while (node->next) node = node->next;
        node->next = out;
    }

    /* Node that we should process retur code in the future */
    if (flag == RUN_IMMEDIATELY)
        ResumeThread(out->handle);
    
    object->counts++;
    thp_handle_unlock(watcher);
    if (new_info)
        *new_info = out;

    return 0;
}

/*
 * Watcher thread.
 *
 * Layer: 2
 * Locking: no
 * 
 * Watcher manages the pressure of thread pool and decide when to terminate
 * a thread which is blocked or not necessary
 */
unsigned int WINAPI watcher_thread(void* param)
{
    thp_handle_t* watcher = (thp_handle_t*)param;
    THP_THREAD_WATCHER* watcher_object = thp_get_watcher_object(watcher);
    THP_POOL* pool_obj = thp_get_pool_object(watcher_object->parent);
    THP_TASK_SERVER* task_server = thp_get_task_server_object(pool_obj->task_server);
    lbs_status_t rc;

    /* This var never changes until pool is destructed, so we save it directly in a local var */
    int pressure_limit = watcher_object->pressure_limit;
    unsigned int count;

    while (1) {
        /* This signal is a shared resource. It can be change only in invoker functions */
        if (watcher_object->status == THP_WATCHER_TERM)
            break;
        
        /* The watcher control the heart beat */
        watcher_object->watcher->heart++;

        /* Here handle high pressure case */
        rc = thp_get_task_count(pool_obj->task_server, &count);
        if (rc == LBS_CODE_INV_HANDLE) {
            /* Terminate the thread if task server is broken */
            return 0;
        }
        if (count > ((pressure_limit*watcher_object->counts)/100) && watcher_object->counts <=69) {
            THP_THREAD_INFO* info;
            rc = thp_create_worker_thread(watcher, RUN_IMMEDIATELY, &info);
        }
        
        /* For now, watcher doesn't run full power.
         * Becuase there is no necessary and heart beat won't raise too fast.
         */
        Sleep(THP_WATCHER_HEART_BASE);
    }

    return 0;
}

/*
 * Create a watcher thread for one watcher obj
 *
 * Layer: 3
 * Locking: yes
 * The watcher thread would always be blocked when it's created
 */
lbs_status_t thp_create_watcher_thread(thp_handle_t* watcher)
{
    lbs_status_t rc;
    THP_THREAD_INFO* threadinfo;
    THP_THREAD_WATCHER* object = thp_get_watcher_object(watcher);

    if (object->watcher)
        return LBS_CODE_WATCHER_EXIST;

    rc = thp_handle_wlock(watcher);
    if (rc != 0) {
        return rc;
    }

    threadinfo = (THP_THREAD_INFO*)calloc(1, sizeof(THP_THREAD_INFO));
    if (!threadinfo) {
        thp_handle_unlock(watcher);
        return LBS_CODE_NO_MEMORY;
    }

    threadinfo->heart = 0;
    threadinfo->next = 0;

    threadinfo->handle = CreateThread(0, 0, watcher_thread, watcher, CREATE_SUSPENDED, &threadinfo->id);
    if (threadinfo->handle == INVALID_HANDLE_VALUE) {
        free(threadinfo);
        thp_handle_unlock(watcher);
        return LBS_CODE_WIN_ERROR;
    }

    object->watcher = threadinfo;

    thp_handle_unlock(watcher);
    return LBS_CODE_OK;
}

/*
 * Start all threads under watcher
 *
 * Layer: 3
 * Locking: yes
 */
lbs_status_t thp_watcher_start_all(thp_handle_t* watcher)
{
    THP_THREAD_INFO* node;
    THP_THREAD_WATCHER* object;
    lbs_status_t rc;

    rc = thp_handle_wlock(watcher);
    if (rc != 0) {
        return rc;
    }

    object = thp_get_watcher_object(watcher);
    node = object->threads;

    while (node) {
        rc = thp_resume_thread(node);
        if (rc != LBS_CODE_OK) {
            /* @@@ What I should do here is to process the error cases.
             * Maybe I can recreate the thread if there is something wrong when creating(no way :) )
             * Or just discard this node and record the error. Anyway watcher will append thread if
             * performance is not enough.
             */
            _asm { int 3 };
            thp_handle_unlock(watcher);
            return rc;
        }
        node = node->next;
    }
    rc = thp_resume_thread(object->watcher);
    if (rc != LBS_CODE_OK) {
        /* Same with front case */
        _asm {int 3};
        thp_handle_unlock(watcher);
        return rc;
    }

    thp_handle_unlock(watcher);

    return 0;
}

/*
 * -------------------- Externs ---------------------
 */

/*
 * Create new watcher obj
 *
 * Layer: 4
 * Locking: no
 */
lbs_status_t thp_new_watcher(
    thp_handle_t* parent,
    int pressure_limit,
    int threads, 
    WORKER_FUNC worker,
    void* param,
    thp_handle_t** watcher
    )
{
    lbs_status_t rc = LBS_CODE_INTERNAL_ERR;
    thp_handle_t* out;
    THP_THREAD_WATCHER* object;
    int i = 0;


    out = thp_new_handle(1);
    if (!out)
        return LBS_CODE_NO_MEMORY;

    object = (THP_THREAD_WATCHER*)calloc(1, sizeof(THP_THREAD_WATCHER));
    if (object == 0) {
        thp_del_handle(out);
        return LBS_CODE_NO_MEMORY;
    }
    out->obj = object;
    object->parent = parent;
    object->pressure_limit = pressure_limit;
    object->param = param;

    if (threads == 0) {
        threads = THP_DEFAULT_THREAD_COUNT;
    }

    for (; i < threads; i++) {
        rc = thp_create_worker_thread(out, SUSPEND, 0);
        if (0 != rc) {
            thp_del_handle(out);
            free(object);
            return rc;
        }
    }

    rc = thp_create_watcher_thread(out);
    if (rc != LBS_CODE_OK){
        thp_del_handle(out);
        free(object);
        return rc;
    }

    object->status = THP_WATCHER_READY;
    object->real_worker = worker;

    *watcher = out;

    return LBS_CODE_OK;
}

/*
 * Start all threads in watcher
 * 
 * Layer: 4
 * Locking: no
 */
lbs_status_t thp_start_threads_and_watcher(thp_handle_t* watcher)
{
    THP_THREAD_WATCHER* object;

    object = thp_get_watcher_object(watcher);
    if (object == 0) {
        return LBS_CODE_INV_HANDLE;
    }

    if (object->status == THP_WATCHER_NULL)
        return LBS_CODE_WATCHER_NOT_READY;

    if (object->status == THP_WATCHER_RUN)
        return LBS_CODE_OK;

    /* This function may fail in the future. Just record here */
    thp_watcher_start_all(watcher);

    return LBS_CODE_OK;
}

/*
 * Stop/Pause all threads in a watcher
 *
 * Layer: 4
 * Locking: yes
 */
lbs_status_t thp_stop_threads_and_watcher(thp_handle_t* watcher)
{
    lbs_status_t rc;
    THP_THREAD_WATCHER* object;
    THP_THREAD_INFO* node;

    object = thp_get_watcher_object(watcher);

    rc = thp_handle_wlock(watcher);
    if (rc != 0) {
        return rc;
    }

    object->status = THP_WATCHER_TERM;
    object->watcher->heart = 0;

    if (object->watcher->handle) {
        WaitForSingleObject(object->watcher->handle, INFINITE);
        CloseHandle(object->watcher->handle);
        object->watcher->handle = 0;
    }

    node = object->threads;
    while (node) {
        if (node->handle) {
            rc = WaitForSingleObject(node->handle, INFINITE);
            CloseHandle(node->handle);
            node->handle = 0;
        }
        node = node->next;
    }

    thp_handle_unlock(watcher);

    return 0;
}

lbs_status_t thp_puase_threads_and_watcher(thp_handle_t* watcher)
{
    lbs_status_t rc;
    THP_THREAD_INFO* node;
    THP_THREAD_WATCHER* object;

    object = thp_get_watcher_object(watcher);

    rc = thp_handle_wlock(watcher);
    if (rc != 0) {
        return rc;
    }

    rc = thp_suspend_thread(object->watcher);
    if (rc != LBS_CODE_OK) {
        thp_handle_unlock(watcher);
        return rc;
    }

    object->status = THP_WATCHER_PUASE;
    object->watcher->heart = 0;

    node = object->threads;
    while (node) {
        rc = thp_suspend_thread(node);
        if (rc != LBS_CODE_OK) {
            /* I don't know how to handle this error here.
             * Maybe better ways would come out from mind when using it.
             */
            __asm { int 3 };
        }

        node->heart = 0;
        node = node->next;
    }

    thp_handle_unlock(watcher);

    return 0;
}

lbs_status_t thp_resume_threads_and_watcher(thp_handle_t* watcher)
{
    return 0;
}

/*
 * Delete watcher and release memeory
 *
 * Layer: 4
 * Locking: yes
 */
lbs_status_t thp_delete_watcher(thp_handle_t* watcher)
{
    THP_THREAD_INFO* node;
    THP_THREAD_WATCHER* object;
    lbs_status_t rc;
    lbs_bool_t is_ready = LBS_FALSE;

    object = thp_get_watcher_object(watcher);

    rc = thp_handle_wlock(watcher);
    if (rc != 0) {
        return rc;
    }

    if (object->status == THP_WATCHER_READY)
        is_ready = LBS_TRUE;
    object->status = THP_WATCHER_TERM;
    if (!is_ready && object->watcher) {
        if (WAIT_OBJECT_0 != WaitForSingleObject(object->watcher->handle, 1000)) {
            TerminateThread(object->watcher->handle, 0);   
        }
    }
    free(object->watcher);

    node = object->threads;
    while (node) {
        THP_THREAD_INFO* temp;
        if (!is_ready && WAIT_OBJECT_0 != WaitForSingleObject(node->handle, 1000)) {
            TerminateThread(node->handle, 0);   
        }
        temp = node;
        node = node->next;
        free(temp);
    }

    watcher->alive = 0;
    watcher->obj = 0;
    free(object);

    thp_handle_unlock(watcher);
    return 0;
}

THP_THREAD_INFO* thp_get_first_thread_in_watcher(thp_handle_t* watcher)
{
    THP_THREAD_INFO* out = 0;
    THP_THREAD_WATCHER* object = thp_get_watcher_object(watcher);
    lbs_status_t rc;

    rc = thp_handle_rlock(watcher);
    if (rc != 0) {
        return 0;
    }

    if (object) {
        out = object->threads;
    }

    thp_handle_unlock(watcher);

    return out;
}
THP_THREAD_INFO* thp_find_thread_in_watcher(thp_handle_t* watcher, THP_THREAD_INFO* target)
{
    THP_THREAD_INFO* out = 0;
    THP_THREAD_WATCHER* object = thp_get_watcher_object(watcher);
    lbs_status_t rc;

    rc = thp_handle_rlock(watcher);
    if (rc != 0) {
        return 0;
    }

    out = object->threads;
    while (out) {
        if (out == target) {
            goto find_and_out;
        }
        out = out->next;
    }
    out = 0;
find_and_out:
    thp_handle_unlock(watcher);

    return out;
}

