#include <stdlib.h>
#include "enet_types.h"
#include <lbs_rwlock.h>
#include "enet_handle.h"

enet_handle_t* g_reuse_table[ENET_HANDLE_MAX_REUSE_TABLE+1];
int g_reuse_location = 0;

enet_handle_t* g_enet_handle_pool = 0;
int g_enet_handle_init = 0;
lbs_lock_t* g_enet_handle_db_lock = 0;

/**
 * Initail global locks
 */
INTERNALCALL
void  enet_handle_management_init()
{
    if (g_enet_handle_init == 0) {
        if (0 == lbs_rwlock_init(&g_enet_handle_db_lock))
            g_enet_handle_init = 1;
    }
}

/**
 * Lock the handle table
 */
INTERNALCALL
void  enet_handle_db_lock()
{
    lbs_rwlock_wlock(g_enet_handle_db_lock);
}

/**
 * Unlock the handle table
 */
INTERNALCALL
void  enet_handle_db_unlock()
{
    lbs_rwlock_unlock(g_enet_handle_db_lock);
}

/**
 * Try to find a handle value in table.
 * 
 */
INTERNALCALL
enet_handle_t* enet_find_handle_in_pool(enet_handle_t* handle)
{
    enet_handle_t* node = g_enet_handle_pool;

    while (node) {
        if (node == handle) {
            break;
        }
        node = node->next;
    }
    return node;
}

/**
 * Append handle value to the tail
 */
INTERNALCALL
void enet_append_handle(enet_handle_t* handle)
{
    enet_handle_t* node;

    if (g_enet_handle_pool == 0) {
        g_enet_handle_pool = handle;
        //enet_handle_db_unlock();
        return;
    }

    node = g_enet_handle_pool;
    while (node->next)
        node = node->next;

    node->next = handle;
    handle->prev = node;
}

/**
 * Remove handle from the handle pool.
 * @description:
 *     this function will not free the memory of the handle and the lock
 * @input: pointer of handle
 */
INTERNALCALL void enet_handle_remove_from_pool(enet_handle_t* handle)
{
    enet_handle_t* node;

    if (handle == 0)
        return;

    node = enet_find_handle_in_pool(handle);

    if (node) {
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
    }
}

INTERNALCALL
void* enet_reuse_handle(enet_handle_t* handle)
{
    void* out = 0;

    /* Actually we dont expact location is greater than max value */
    if (g_reuse_location >= ENET_HANDLE_MAX_REUSE_TABLE) {
        /* Reuse table is full. Discard this one */
        out = handle->lock;
        free(handle);
    } else {
        g_reuse_location++;
        g_reuse_table[g_reuse_location] = handle;
    }

    return out;
}

int enet_is_handle_referenced(enet_handle_t* handle)
{
    return handle->refcnt;
}

int enet_is_handle_alive(enet_handle_t* handle)
{
    return handle->alive;
}

enet_handle_t* enet_new_handle(int need_lock)
{
    enet_handle_t* out = 0;
    int lock_is_reused = 0;

    enet_handle_management_init();

    if (g_reuse_location == 0) {
        out = (enet_handle_t*)calloc(1, sizeof(enet_handle_t));
        if (!out)
            return out;
    } else {
        enet_handle_db_lock();
        out = g_reuse_table[g_reuse_location];
        
        out->next = 0;
        out->obj = 0;
        out->prev = 0;
        out->refcnt = 0;

        g_reuse_table[g_reuse_location] = 0;
        g_reuse_location--;
        lock_is_reused = 1;
        enet_handle_db_unlock();
    }

    /* If locked is 1, the handle is greped from reuse list. So we don't init the lock */
    if (need_lock && !lock_is_reused) {
        if (lbs_rwlock_init(&((lbs_lock_t*)out->lock))) {
            free(out);
            out = 0;
        }
    }

    if (out) {
        enet_handle_db_lock();
        enet_append_handle(out);
        enet_handle_db_unlock();
        out->alive = 1;
    }


    return out;
}

enet_handle_t* enet_new_handle_ex(void* object, int need_lock)
{
    enet_handle_t* out = enet_new_handle(need_lock);

    if (out) {
        out->obj = object;
    }
    return out;
}

/* Delete the handle and cleanup.
 * Before we make this operation we must make sure there are no
 * concurrent accesses to this handle.
 */
void enet_delete_handle(enet_handle_t* handle)
{
    void* lock = 0;

    if (!handle) {
        return;
    }    

    lbs_rwlock_wlock((lbs_lock_t*)handle->lock);

    if (!enet_is_handle_alive(handle)) {
        lbs_rwlock_unlock((lbs_lock_t*)handle->lock);
        return;
    }
    handle->alive = 0;

    if (!enet_is_handle_referenced(handle)) {
        enet_handle_remove_from_pool(handle);
        lock = enet_reuse_handle(handle);
    }

    if (lock) {
        lbs_rwlock_unlock((lbs_lock_t*)lock);
        lbs_rwlock_destroy(&((lbs_lock_t*)lock));
    } else {
        lbs_rwlock_unlock((lbs_lock_t*)handle->lock);
    }
}

/* Unlock this handle and leave access right to other requests.
 * If this handle is terminate and reference count is zero, delete it.
 */
lbs_status_t enet_handle_unlock(enet_handle_t* handle)
{
    int is_alive;

    if (handle == 0)
        return LBS_CODE_INV_HANDLE;
    
    is_alive = handle->alive;

    lbs_rwlock_unlock((lbs_lock_t*)(handle->lock));

    enet_handle_db_lock();

    if (enet_is_handle_referenced(handle))
        handle->refcnt--;

    if (is_alive == 0 && handle->refcnt <= 0) {
        enet_delete_handle(handle);
    }

    enet_handle_db_unlock();

    return LBS_CODE_OK;
}

lbs_status_t enet_handle_rlock(enet_handle_t* handle)
{
    if (handle == 0)
        return LBS_CODE_INV_HANDLE;

    enet_handle_db_lock();

    if (0 == enet_find_handle_in_pool(handle)) {
        enet_handle_db_unlock();
        return LBS_CODE_INV_HANDLE;
    }
    handle->refcnt++;

    enet_handle_db_unlock();

    lbs_rwlock_rlock((lbs_lock_t*)(handle->lock));
    
    if (handle->alive == 0 || handle->obj == 0) {
        enet_handle_unlock(handle);
        return LBS_CODE_INV_HANDLE;
    }

    return LBS_CODE_OK;
}

lbs_status_t enet_handle_wlock(enet_handle_t* handle)
{
    if (handle == 0)
        return LBS_CODE_INV_PARAM;

    enet_handle_db_lock();

    if (0 == enet_find_handle_in_pool(handle)) {
        enet_handle_db_unlock();
        return LBS_CODE_INV_HANDLE;
    }
    handle->refcnt++;

    enet_handle_db_unlock();

    lbs_rwlock_wlock((lbs_lock_t*)(handle->lock));

    if (handle->alive == 0 || handle->obj == 0) {
        enet_handle_unlock(handle);
        return LBS_CODE_INV_HANDLE;
    }

    return LBS_CODE_OK;
}

int enet_is_valid_handle(enet_handle_t* handle)
{
    enet_handle_t* node;

    if (handle == 0) {
        return 0;
    }

    enet_handle_db_lock();

    node = g_enet_handle_pool;

    while (node) {
        if (node == handle) {
            enet_handle_db_unlock();
            return 1;
        }
        node = node->next;
    }

    enet_handle_db_unlock();

    return 0;
}

/* Free all the reuse things*/
void enet_free_reuse_handle()
{
    enet_handle_t* node;

    enet_handle_db_lock();

    for (; 0 <= g_reuse_location; g_reuse_location--) {
        node = g_reuse_table[g_reuse_location];
        lbs_rwlock_destroy(&((lbs_lock_t*)node->lock));
        free(node);
        g_reuse_table[g_reuse_location] = 0;
    }

    enet_handle_db_unlock();
}
