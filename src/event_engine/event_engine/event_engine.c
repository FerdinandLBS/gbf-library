#include <event_engine.h>
#include <lbs_thread.h>
#include <lbs_rwlock.h>

#ifdef _DEBUG
#include <stdio.h>
#include <assert.h>
#endif

//#define USE_STACK

lbs_thread_t __event_engine_thread = 0;
lbs_thread_t __event_timer_thread = 0;

lbs_lock_t* __event_engine_db_lock = 0;
lbs_lock_t* __event_timer_db_lock = 0;

fn_event_disptcher __event_dispatcher = 0;

int __ee_is_terminated = 0;
int __event_iterator = 0;
int __event_head = 0;
int __event_tail = 0;
int __event_is_ready = 0;
int __event_item_count = 0;
int __event_total_handler = 0;

typedef struct __event_engine_info_st {
    void* object;
    void* param1;
    void* param2;
    unsigned id;
}EVENT_ENGINE_INFO;

typedef struct __event_timer_info_st {
    unsigned tick;
    unsigned expired;
    unsigned id;
    void* param;
    fn_timer_callback callback;

    struct __event_timer_info_st* prev;
    struct __event_timer_info_st* next;
}EVENT_TIMER_INFO;

#ifdef USE_STACK
EVENT_ENGINE_INFO __event_engine_db[DEFAULT_EVENT_DB_SIZE] = {0};
#else
EVENT_ENGINE_INFO* __event_engine_db = 0;
#endif

EVENT_TIMER_INFO* __event_first_timer = 0;

#define event_engine_db_lock() lbs_rwlock_wlock(__event_engine_db_lock)
#define event_engine_db_unlock() lbs_rwlock_unlock(__event_engine_db_lock)

#define event_timer_db_lock() lbs_rwlock_wlock(__event_timer_db_lock)
#define event_timer_db_unlock() lbs_rwlock_unlock(__event_timer_db_lock)

#define EE_MAX_TIMER_ONE_ROUND 20

static EVENT_TIMER_INFO* ee_new_timer_node(unsigned tick, unsigned id, void* param, fn_timer_callback callback)
{
    EVENT_TIMER_INFO* out = (EVENT_TIMER_INFO*)malloc(sizeof(EVENT_TIMER_INFO));
    if (out == 0)
        return 0;

    out->tick = tick;
    out->id = id;
    out->expired = GetTickCount()+tick;
    out->param = param;
    out->callback = callback;
    out->prev = 0;
    out->next = 0;

    return out;
}

static __inline void ee_reset_timer_node_nl(EVENT_TIMER_INFO* node)
{
    node->expired = GetTickCount() + node->tick;
}

static void ee_insert_timer_node_nl(EVENT_TIMER_INFO* node)
{
    if (!__event_first_timer) {
        __event_first_timer = node;
        node->prev = node;
        node->next = node;
    } else {
        EVENT_TIMER_INFO* next = __event_first_timer->next;
        node->prev = __event_first_timer;
        __event_first_timer->next = node;
        next->prev = node;
        node->next = next;
    }
}

static EVENT_TIMER_INFO* ee_delete_timer_node_nl(EVENT_TIMER_INFO* node)
{
    EVENT_TIMER_INFO* iterate = 0;

    if (node->prev == node) {
        // Only one node in list
        free(node);
        __event_first_timer = 0;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (node == __event_first_timer)
            __event_first_timer = node->next;

        iterate = node->prev;
        free(node);
    }

    return iterate;
}

/**
 * Get event from the head, if there is no, pending
 * @description:
 *     null
 * @output: pointer of event info
 * @return: true or false
 */
static int pop_event(EVENT_ENGINE_INFO* event_info)
{
    int result = 0;
    int spin = 50;

    while (1) {
        /* check event queue */
        event_engine_db_lock();
        if (__event_item_count > 0)
            break;
        event_engine_db_unlock();

        /* we spin here to improve performance.
         * if we dont have task then release the CPU time slice.
         */
        if (spin < 0)
            Sleep(1);
        else
            spin--;

        if (__ee_is_terminated)
            return 1;
    }

    event_info->id = __event_engine_db[__event_head].id;
    event_info->object = __event_engine_db[__event_head].object;
    event_info->param1 = __event_engine_db[__event_head].param1;
    event_info->param2 = __event_engine_db[__event_head].param2;

    __event_engine_db[__event_head].id = 0;

    if (__event_item_count > 1)
        if (__event_head < DEFAULT_EVENT_DB_SIZE - 1) {
            __event_head++;
        } else {
            __event_head = 0;
        }
#ifdef _DEBUG
    else
        assert(__event_head == __event_tail);
#endif

    result = 1;
    __event_item_count--;

    event_engine_db_unlock();

    return result;
}

/* Timer event worker thread */
unsigned LBS_THREAD_CALL ee_timer_event(void* param)
{
    unsigned tick;
    int max_count = EE_MAX_TIMER_ONE_ROUND;
    int rc;
    EVENT_TIMER_INFO* node;

    while (!__ee_is_terminated) {
        /* time granularity */
        Sleep(50);

        tick = GetTickCount();

        event_timer_db_lock();

        node = __event_first_timer;

        while (node) {
            if (node->expired <= tick) { // timer triggered
                if (node->callback) {
                    rc = node->callback(node->param, node->id);
                    if (rc == EE_TIMER_RESET) {
                        ee_reset_timer_node_nl(node); 
                    } else {
                        node = ee_delete_timer_node_nl(node);
                        if (!node) break;
                    }
                }
            }
            node = node->next;
            max_count--;
            if (max_count == 0 || node == __event_first_timer) {
                // we have a lot of timers. but we have rest here and start next round
                // we have few timers. so we could stay in a while
                __event_first_timer = node;
                max_count = EE_MAX_TIMER_ONE_ROUND;
                break;
            }
        }

        event_timer_db_unlock();
    }

    return 0;
}

/**
 * Process all the events in this thread
 * @description:
 *     null
 * @input: param;
 * @return: thread exit code
 */
unsigned LBS_THREAD_CALL event_engine_handler(void* param)
{
    EVENT_ENGINE_INFO event_info;

    while (!__ee_is_terminated) {
        pop_event(&event_info);

        if (event_info.id != 0) {
            __event_dispatcher(event_info.object, event_info.param1, event_info.param2, event_info.id);
        }
    }

    return 0;
}

lbs_status_t LBS_CALLCONV ee_init_event(fn_event_disptcher dispatcher)
{
    int result;
    lbs_status_t st, st2;

    if (__event_is_ready) {
        return LBS_CODE_OK;
    }
    if (dispatcher == 0) {
        return LBS_CODE_INV_PARAM;
    }
    
    result = lbs_rwlock_init(&__event_engine_db_lock);
    if (result != 0) {
        return LBS_CODE_NO_MEMORY;
    }

    result = lbs_rwlock_init(&__event_timer_db_lock);
    if (result != 0) {
        lbs_rwlock_destroy(&__event_engine_db_lock);
        __event_engine_db_lock = 0;
        return LBS_CODE_NO_MEMORY;
    }

#ifndef USE_STACK
    __event_engine_db = (EVENT_ENGINE_INFO*)malloc(sizeof(EVENT_ENGINE_INFO)*DEFAULT_EVENT_DB_SIZE);
    if (__event_engine_db == 0) {
        lbs_rwlock_destroy(&__event_engine_db_lock);
        __event_engine_db_lock = 0;
        return LBS_CODE_NO_MEMORY;
    }
#endif

    __event_engine_db[0].id = 0;

    st = lbs_create_thread(0, event_engine_handler, 0, &__event_engine_thread);
    st2 = lbs_create_thread(0, ee_timer_event, 0, &__event_timer_thread);
    if (st != LBS_CODE_OK || st2 != LBS_CODE_OK) {
        lbs_rwlock_destroy(&__event_engine_db_lock);
        lbs_rwlock_destroy(&__event_timer_db_lock);
        __event_engine_db_lock = 0;
        __event_timer_db_lock = 0;

        // todo: handle survival thread
#ifndef USE_STACK
        free(__event_engine_db);
        __event_engine_db = 0;
#endif
        return st;
    }

    __event_dispatcher = dispatcher;
    __event_is_ready = 1;

    return LBS_CODE_OK;
}

/* Start a timer */
lbs_status_t LBS_CALLCONV ee_timer(unsigned ms, void* param, unsigned id, fn_timer_callback callback)
{
    lbs_status_t rc = LBS_CODE_OK;
    EVENT_TIMER_INFO* node;

    if (ms == 0 || callback == 0)
        return LBS_CODE_INV_PARAM;

    node = ee_new_timer_node(ms, id, param, callback);
    if (!node)
        return LBS_CODE_NO_MEMORY;

    event_timer_db_lock();

    ee_insert_timer_node_nl(node);

    event_timer_db_unlock();


    return rc;
}

lbs_status_t LBS_CALLCONV ee_post_event(void* object, void* param1, void* param2, unsigned event_id)
{
    lbs_status_t st = LBS_CODE_INTERNAL_ERR;

    if (!object || event_id == 0)
        return LBS_CODE_INV_PARAM;

    event_engine_db_lock();

    /* Data base is full */
    if (__event_item_count == DEFAULT_EVENT_DB_SIZE)
        goto escape;
    
    /* If the event count eq zero, tail should not move */
    if (__event_item_count != 0)
        if (__event_tail < DEFAULT_EVENT_DB_SIZE - 1) {
            __event_tail++;
        } else {
            __event_tail = 0;
        }

    __event_engine_db[__event_tail].id = event_id;
    __event_engine_db[__event_tail].object = object;
    __event_engine_db[__event_tail].param1 = param1;
    __event_engine_db[__event_tail].param2 = param2;
    __event_item_count++;
    __event_total_handler++;

    st = LBS_CODE_OK;

escape:
    event_engine_db_unlock();

    return st;
}


