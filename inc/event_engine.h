#ifndef _EVENT_ENGINE_H_
#define _EVENT_ENGINE_H_

#include <lbs.h>

#define DEFAULT_EVENT_DB_SIZE 2000

typedef lbs_status_t (LBS_CALLCONV *fn_event_disptcher)(void* object, void* param1, void* param2, unsigned event_id);

#ifdef __cplusplus 
extern "C" {
#endif

    lbs_status_t LBS_CALLCONV ee_init_event(fn_event_disptcher dispatcher);

    lbs_status_t LBS_CALLCONV ee_post_event(void* object, void* param1, void* param2, unsigned event_id);

#ifdef __cplusplus
};
#endif

#endif //__ENET_EVENT_H__
