#include <event_engine.h>
#include <lbs_thread.h>
#include <lbs_rwlock.h>

#ifdef _DEBUG
#include <stdio.h>
#include <assert.h>
#endif

//#define USE_STACK

lbs_thread_t __event_engine_thread = 0;
lbs_lock_t* __event_engine_db_lock = 0;
fn_event_disptcher __event_dispatcher = 0;
int __event_iterator = 0;
int __event_head = 0;
int __event_tail = 0;
int __event_is_ready = 0;
int __event_item_count = 0;
int __event_total_handler = 0;

typedef struct __event_engine_info {
    void* object;
    void* param1;
    void* param2;
    unsigned id;
}EVENT_ENGINE_INFO;

#ifdef USE_STACK
EVENT_ENGINE_INFO __event_engine_db[DEFAULT_EVENT_DB_SIZE] = {0};
#else
EVENT_ENGINE_INFO* __event_engine_db = 0;
#endif

#define event_engine_db_lock() lbs_rwlock_wlock(__event_engine_db_lock)
#define event_engine_db_unlock() lbs_rwlock_unlock(__event_engine_db_lock)

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

continue_waiting:
    while (__event_item_count == 0) {
        /*
         * For a real server, Sleep(0) or SwitchToThread() is OK. We dont consider the CPU too much.
         * But in normal APP, Sleep(1) is better and releases much more CPU resource.
         */
        Sleep(1);
    }

    event_engine_db_lock();

	if (__event_item_count == 0) {
		event_engine_db_unlock();
		goto continue_waiting;
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

    while (1) {
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
    lbs_status_t st;

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
    if (st != LBS_CODE_OK) {
        lbs_rwlock_destroy(&__event_engine_db_lock);
        __event_engine_db_lock = 0;

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


