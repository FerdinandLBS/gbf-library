#include <Windows.h>
#include "lbs_rwlock.h"

/*
 * This is the structure of threads lock.
 * For now, there are two kinds of locks.
 * Read/Write lock:
 *     This lock is used for resource protection.
 * Main Control lock:
 *     This lock is used for ONE time access. Even the thread
 *     who has already get the access of lock it cannot get
 *     in again unless the lock is unlocked.
 */
typedef struct _lbs_rwlock_struct {
    /* Real lock */
    CRITICAL_SECTION lock;

    /* Reader counter */
    int reader;

    /* Writer counter. This value can be only 0 or 1 */
    int writer;

    /* Used for MC lock */
    int main_ctrl;
}lbs_lock_t;

#define LBS_LOCK_MASK 0x32443244

/**
 * Lock id and lock object pointer convertion.
 * @@@ Know that this is a very simple method to convert two types.
 * @@@ We should write a better and common function to do this.
 */
static lbs_lock_t* lbs_lockid_to_handle(lbs_rwlock_t lockid)
{
    if (lockid == 0) {
        return 0;
    }

    return (lbs_lock_t*)(lockid^LBS_LOCK_MASK);
}
static lbs_rwlock_t lbs_handle_to_lockid(lbs_lock_t* handle)
{
    if (handle == 0) {
        return 0;
    }

    return ((lbs_rwlock_t)handle)^LBS_LOCK_MASK;
}

/* Create new lock*/
lbs_lock_t* lbs_new_rwlock()
{
    return (lbs_lock_t*)calloc(1, sizeof(lbs_lock_t));
}

/* Initialize a rwlock */
int lbs_rwlock_init(lbs_rwlock_t* lock)
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

    *lock = lbs_handle_to_lockid(p);

    return 0;
}

/* Initialize a mc lock */
int lbs_mc_lock_init(lbs_mc_lock_t* lock)
{
    return lbs_rwlock_init(lock);
}

/* Get a reader lock */
int lbs_rwlock_rlock(lbs_rwlock_t lock)
{
    lbs_lock_t* p;

    p = lbs_lockid_to_handle(lock);
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
int lbs_rwlock_wlock(lbs_rwlock_t lock)
{
    lbs_lock_t* p;

    p = lbs_lockid_to_handle(lock);
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
int lbs_mc_lock(lbs_mc_lock_t lock)
{
    lbs_lock_t* p;
    int thread_ctrl;

    p = lbs_lockid_to_handle(lock);
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
int lbs_mc_unlock(lbs_mc_lock_t lock)
{
    lbs_lock_t* p;

    p = lbs_lockid_to_handle(lock);
    if (p == 0 || lock == 0) {
        return 1;
    }

    p->main_ctrl = 0;
    LeaveCriticalSection(&(p->lock));
    
    return 0;
}

/* Release a rwlock */
int lbs_rwlock_unlock(lbs_rwlock_t lock)
{
    lbs_lock_t* p;

    p = lbs_lockid_to_handle(lock);
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
void lbs_rwlock_destroy(lbs_rwlock_t* lock)
{
    lbs_lock_t* p;

    p = lbs_lockid_to_handle(*lock);
    if (p == 0 || lock == 0) {
        return;
    }

    DeleteCriticalSection(&p->lock);
    free(p);
    *lock = 0;
}

/* Free a mc lock object */
void lbs_mc_lock_destroy(lbs_mc_lock_t* lock)
{
    lbs_rwlock_destroy(lock);
}
