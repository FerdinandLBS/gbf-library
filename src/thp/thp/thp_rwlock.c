#include <Windows.h>
#include "thp_rwlock.h"

/* Create new lock*/
thp_lock_t* thp_new_rwlock()
{
    return (thp_lock_t*)calloc(1, sizeof(thp_lock_t));
}

/* Initialize a rwlock */
int thp_rwlock_init(thp_lock_t** lock)
{
    thp_lock_t* p;

    if (lock == 0) {
        return 1;//Error
    }

    p = thp_new_rwlock();
    if (p == 0) {
        return 1;//Error
    }

    InitializeCriticalSection(&(p->lock));

    *lock = p;

    return 0;
}

/* Initialize a mc lock */
int thp_mc_lock_init(thp_lock_t** lock)
{
    return thp_rwlock_init(lock);
}

/* Get a reader lock */
int thp_rwlock_rlock(thp_lock_t* lock)
{
    thp_lock_t* p;

    p = lock;
    if (p == 0 || lock == 0) {
        return 1;
    }

    EnterCriticalSection(&(p->lock));

    if (p->writer != 0) {
        LeaveCriticalSection(&(p->lock));
        return 1;
    }

    InterlockedIncrement(&p->reader);

    LeaveCriticalSection(&(p->lock));

    return 0;
}

/* Get a writer lock */
int thp_rwlock_wlock(thp_lock_t* lock)
{
    thp_lock_t* p;

    p = (lock);
    if (p == 0 || lock == 0) {
        return 1;
    }

    EnterCriticalSection(&(p->lock));

    /* We allow the same thread to hold the lock more than once */
    if (p->writer != 0) {
        return 0;
    }

    while (1) {
        if (p->reader == 0) {
            p->writer = 1;
            return 0;
        }
        SwitchToThread();
    }
}

/* Get a mc lock */
int thp_mc_lock(thp_lock_t* lock)
{
    thp_lock_t* p;
    int thread_ctrl;

    p = (lock);
    if (p == 0 || lock == 0) {
        return 1;
    }

get_thrad_lock:
    thread_ctrl = 0;
    EnterCriticalSection(&(p->lock));
    while (p->main_ctrl) {
        SwitchToThread();
        thread_ctrl = 1;
    }
    
    if (thread_ctrl == 1)
        goto get_thrad_lock;

    p->main_ctrl = 1;

    return 0;
}

/* Release a mc lock */
int thp_mc_unlock(thp_lock_t* lock)
{
    thp_lock_t* p;

    p = (lock);
    if (p == 0 || lock == 0) {
        return 1;
    }

    p->main_ctrl = 0;
    LeaveCriticalSection(&(p->lock));
    
    return 0;
}

/* Release a rwlock */
int thp_rwlock_unlock(thp_lock_t* lock)
{
    thp_lock_t* p;

    p = (lock);
    if (p == 0 || lock == 0) {
        return 1;
    }

    if (p->writer == 0) {
        InterlockedDecrement(&p->reader);
    } else {
        if (p->lock.RecursionCount == 1) {
            p->writer = 0;
        }
        LeaveCriticalSection(&(p->lock));
    }

    return 0;
}

/* Free a rwlock object */
void thp_rwlock_destroy(thp_lock_t** lock)
{
    thp_lock_t* p;

    p = (*lock);
    if (p == 0 || lock == 0) {
        return;
    }

    DeleteCriticalSection(&p->lock);
    free(p);
    *lock = 0;
}

/* Free a mc lock object */
void thp_mc_lock_destroy(thp_lock_t** lock)
{
    thp_rwlock_destroy(lock);
}
