#ifndef _EASENET_HANDLE_H_
#define _EASENET_HANDLE_H_

/* 
 * The max reuse table
 * The reuse table will cost additional memory.
 * Try to set the number to a proper value
 */
#define ENET_HANDLE_MAX_REUSE_TABLE 128

#ifdef __cplusplus
extern "C" {
#endif

    int enet_is_handle_referenced(enet_handle_t* handle);
    int enet_is_handle_alive(enet_handle_t* handle);

    /**
     * Create new handle
     * @description: 
     *     Create new handle or grep one from reuse table
     * @input: true if a initialization of lock is needed
     * @return: pointer of the handle
     */
    enet_handle_t* enet_new_handle(int need_lock);

    /**
     * Create new handle extra
     * @description:
     *     null
     * @input: the entity you want to connect
     * @input: true if a initialization of lock is needed
     * @return: pointer of handle
     */
    enet_handle_t* enet_new_handle_ex(void* entity, int need_lock);

    /**
     * Delete the handle
     * @description:
     *     This handle would be reused if the table is not full
     * @input: the pointer of handle
     * @return: null
     */
    void enet_delete_handle(enet_handle_t* handle);

    /**
     * Aquire the read lock of a handle
     * @description:
     *     donot call it multiple times in one thread.
     * @input: pointer of the handle
     * @return: status
     */
    lbs_status_t enet_handle_rlock(enet_handle_t* handle);

    /**
     * Aquire the write lock of a handle
     * @description:
     *     mutex with other locks
     * @input: pointer of the handle
     * @return: status
     */
    lbs_status_t enet_handle_wlock(enet_handle_t* handle);

    /**
     * Unlock and kind of lock
     * @descriptioin:
     *     null
     * @input: pointer of handle
     * @return: status
     */
    lbs_status_t enet_handle_unlock(enet_handle_t* handle);

    /**
     * return 1 if valid. return 0 if invalid
     */
    int enet_is_valid_handle(enet_handle_t* handle);

    /**
     * Release the memory of reused handles
     */
    void enet_free_reuse_handle();

#ifdef __cplusplus
};
#endif


#endif
