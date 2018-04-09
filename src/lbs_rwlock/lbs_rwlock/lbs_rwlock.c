#include "lbs_rwlock.h"

/* Create new lock*/
lbs_lock_t* lbs_new_rwlock()
{
    return (lbs_lock_t*)calloc(1, sizeof(lbs_lock_t));
}

/* Initialize a rwlock */
int lbs_rwlock_init(lbs_lock_t** lock)
{
    lbs_lock_t* p;

    if (lock == 0) {
        return 1;//Error
    }

    p = lbs_new_rwlock();
    if (p == 0) {
        return 1;//Error
    }

    InitializeCriticalSection(&(p->lock));

    *lock = p;

    return 0;
}

/* Initialize a mc lock */
int lbs_mc_lock_init(lbs_lock_t** lock)
{
    return lbs_rwlock_init(lock);
}

/* Get a reader lock */
int lbs_rwlock_rlock(lbs_lock_t* lock)
{
    if (lock == 0) {
        return 1;
    }

    EnterCriticalSection(&(lock->lock));

    if (lock->writer != 0) {
        LeaveCriticalSection(&(lock->lock));
        return 1;
    }

    InterlockedIncrement(&lock->reader);

    LeaveCriticalSection(&(lock->lock));

    return 0;
}

/* Get a writer lock */
int lbs_rwlock_wlock(lbs_lock_t* lock)
{
    if (lock == 0) {
        return 1;
    }

    EnterCriticalSection(&(lock->lock));

    /* We allow the same thread to hold the lock more than once */
    if (lock->writer != 0) {
        return 0;
    }

    while (1) {
        if (lock->reader == 0) {
            lock->writer = 1;
            return 0;
        }
        SwitchToThread();
    }
}

/* Get a mc lock */
int lbs_mc_lock(lbs_lock_t* lock)
{
    int thread_ctrl;

    if (lock == 0) {
        return 1;
    }

get_thrad_lock:
    thread_ctrl = 0;
    EnterCriticalSection(&(lock->lock));
    while (lock->main_ctrl) {
        SwitchToThread();
        thread_ctrl = 1;
    }
    
    if (thread_ctrl == 1)
        goto get_thrad_lock;

    lock->main_ctrl = 1;

    return 0;
}

/* Release a mc lock */
int lbs_mc_unlock(lbs_lock_t* lock)
{
    if (lock == 0) {
        return 1;
    }

    lock->main_ctrl = 0;
    LeaveCriticalSection(&(lock->lock));
    
    return 0;
}

/* Release a rwlock */
int lbs_rwlock_unlock(lbs_lock_t* lock)
{
    if (lock == 0) {
        return 1;
    }

    if (lock->writer == 0) {
        InterlockedDecrement(&lock->reader);
    } else {
        if (lock->lock.RecursionCount == 1) {
            lock->writer = 0;
        }
        LeaveCriticalSection(&(lock->lock));
    }

    return 0;
}

/* Free a rwlock object */
void lbs_rwlock_destroy(lbs_lock_t** lock)
{
    if (lock == 0) {
        return;
    }

    DeleteCriticalSection(&((*lock)->lock));
    free(lock);
    *lock = 0;
}

/* Free a mc lock object */
void lbs_mc_lock_destroy(lbs_lock_t** lock)
{
    lbs_rwlock_destroy(lock);
}
