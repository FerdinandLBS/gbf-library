#include "thp_handle.h"

thp_handle_t* g_handle_pool = 0;
int g_init = 0;
thp_lock_t* g_lock = 0;

void thp_handle_management_init()
{
    if (g_init == 0) {
        if (0 == thp_rwlock_init(&g_lock))
            g_init = 1;
    }
}

void thp_handle_db_lock()
{
    thp_rwlock_wlock(g_lock);
}

void thp_handle_db_unlock()
{
    thp_rwlock_unlock(g_lock);
}

thp_handle_t* thp_find_handle_in_pool(thp_handle_t* handle)
{
    thp_handle_t* node = g_handle_pool;

    while (node) {
        if (node == handle) {
            break;
        }
        node = node->next;
    }
    return node;
}

void thp_append_handle(thp_handle_t* handle)
{
    thp_handle_t* node;

    thp_handle_db_lock();

    if (g_handle_pool == 0) {
        g_handle_pool = handle;
        thp_handle_db_unlock();
        return;
    }

    node = g_handle_pool;
    while (node->next)
        node = node->next;

    node->next = handle;
    handle->prev = node;

    thp_handle_db_unlock();
}

void thp_handle_remove_from_pool(thp_handle_t* handle)
{
    thp_handle_t* node;

    if (handle == 0)
        return;

    node = thp_find_handle_in_pool(handle);

    if (node) {
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
    }
}

thp_handle_t* thp_new_handle(int need_lock)
{
    thp_handle_t* out = 0;

    /* If the freed handle pool is not empty. Return the head */
    if (out)
        return out;

    thp_handle_management_init();

    out = (thp_handle_t*)calloc(1, sizeof(thp_handle_t));
    if (!out)
        return out;

    if (need_lock) {
        if (thp_rwlock_init(&((thp_lock_t*)out->lock))) {
            free(out);
            out = 0;
        }
    }

    if (out) {
        thp_append_handle(out);
        out->alive = 1;
    }

    return out;
}

void thp_handle_inc_ref(thp_handle_t* handle)
{
    InterlockedIncrement(&(handle->refcnt));
}

void thp_handle_dec_ref(thp_handle_t* handle)
{
    InterlockedDecrement(&(handle->refcnt));
}

/* Delete the handle and cleanup.
 * Before we make this operation we must make sure there are no
 * Concurrent accesses to this handle.
 */
void thp_del_handle(thp_handle_t* handle)
{
    if (!handle) {
        return;
    }    

    thp_handle_remove_from_pool(handle);

    thp_rwlock_destroy(&((thp_lock_t*)handle->lock));
    free(handle);
}

/* Unlock this handle and leave access right to other requests.
 * If this handle is terminate and reference count is zero, delete it.
 */
lbs_status_t thp_handle_unlock(thp_handle_t* handle)
{
    int is_alive;

    if (handle == 0)
        return LBS_CODE_INV_HANDLE;
    
    is_alive = handle->alive;

    thp_rwlock_unlock((thp_lock_t*)(handle->lock));

    thp_handle_db_lock();

    InterlockedDecrement(&(handle->refcnt));

    if (is_alive == 0 && handle->refcnt <= 0) {
        thp_del_handle(handle);
    }

    thp_handle_db_unlock();

    return LBS_CODE_OK;
}

lbs_status_t thp_handle_rlock(thp_handle_t* handle)
{
    if (handle == 0)
        return LBS_CODE_INV_HANDLE;

    thp_handle_db_lock();

    if (0 == thp_find_handle_in_pool(handle)) {
        thp_handle_db_unlock();
        return LBS_CODE_INV_HANDLE;
    }

    thp_handle_inc_ref(handle);

    thp_handle_db_unlock();

    thp_rwlock_rlock((thp_lock_t*)(handle->lock));

    if (handle->alive == 0) {
        thp_handle_unlock(handle);
        return LBS_CODE_INV_HANDLE;
    }

    return LBS_CODE_OK;
}

lbs_status_t thp_handle_wlock(thp_handle_t* handle)
{
    if (handle == 0)
        return LBS_CODE_INV_PARAM;

    thp_handle_db_lock();

    if (0 == thp_find_handle_in_pool(handle)) {
        thp_handle_db_unlock();
        return LBS_CODE_INV_HANDLE;
    }

    thp_handle_inc_ref(handle);

    thp_handle_db_unlock();

    thp_rwlock_wlock((thp_lock_t*)(handle->lock));

    if (handle->alive == 0) {
        thp_handle_unlock(handle);
        return LBS_CODE_INV_HANDLE;
    }

    return LBS_CODE_OK;
}
