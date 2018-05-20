#ifndef _EVENT_ENGINE_H_
#define _EVENT_ENGINE_H_

#include <lbs.h>

#define DEFAULT_EVENT_DB_SIZE 2000

/**
 * Use these values as return value of timer callback function
 */
#define EE_TIMER_DONE  0x0000 // timer should be removed after triggered
#define EE_TIMER_RESET 0X0001 // timer should be reuse after triggered

typedef lbs_status_t (LBS_CALLCONV *fn_event_disptcher)(void* object, void* param1, void* param2, unsigned event_id);

typedef lbs_status_t (LBS_CALLCONV *fn_timer_callback)(void* param, unsigned id);

#ifdef __cplusplus 
extern "C" {
#endif

    /**
     * Initialize Event Engine
     * 
     * always call it before and other event engine functions.
     * 
     * input: pointer of auto event callback function
     * return: status
     */
    lbs_status_t LBS_CALLCONV ee_init_event(fn_event_disptcher dispatcher);

    /**
     * Post An Event
     *
     * post an event to handler. the dispatcher function will be called after this function call.
     *
     * input: pointer of posted object
     *        pointer of parameter 1
     *        pointer of parameter 2
     *        event id
     * return: status
     */
    lbs_status_t LBS_CALLCONV ee_post_event(void* object, void* param1, void* param2, unsigned event_id);

    /**
     * Set a timer
     *
     * Start a timer event.
     *
     * !!NOTE: THIS IS NOT AN EXACTLY TIMER WHICH CARES MORE ABOUT PERFORMANCE.
     *
     * input: milli-second
     *        pointer of parameter
     *        timer id
     *        pointer of callback function
     * return: status
     */
    lbs_status_t LBS_CALLCONV ee_timer(unsigned ms, void* param, unsigned id, fn_timer_callback callback);

#ifdef __cplusplus
};
#endif

#endif //__ENET_EVENT_H__
