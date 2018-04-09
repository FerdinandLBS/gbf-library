#ifndef _THP_RWLOCK_H_
#define _THP_RWLOCK_H_

#include <Windows.h>
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
typedef struct _thp_rwlock_struct {
    /* Real lock */
    CRITICAL_SECTION lock;

    /* Reader counter */
    int reader;

    /* Writer counter. This value can be only 0 or 1 */
    int writer;

    /* Used for MC lock */
    int main_ctrl;
}thp_lock_t;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Initialize a rwlock.
     */
    int thp_rwlock_init(thp_lock_t** lock);

    /**
     * Get a read lock.
     */
    int thp_rwlock_rlock(thp_lock_t* lock);

    /**
     * Get a write lock.
     * In the same thread caller can call this funtion more than
     * once, but please unlock all of them always.
     */
    int thp_rwlock_wlock(thp_lock_t* lock);

    /**
     * Unlock a lock
     */
    int thp_rwlock_unlock(thp_lock_t* lock);

    /**
     * Destroy a lock and free the memroy it used.
     */
    void thp_rwlock_destroy(thp_lock_t** lock);

    int thp_mc_lock_init(thp_lock_t** lock);
    int thp_mc_lock(thp_lock_t* lock);
    int thp_mc_unlock(thp_lock_t* lock);
    void thp_mc_lock_destroy(thp_lock_t** lock);

#ifdef __cplusplus
};
#endif

#endif //_LBS_RWLOCK_H_


