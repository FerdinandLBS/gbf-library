#include "event.h"
#include "module.h"
#include "sys.h"
#include <lbs_thread.h>
#include <lbs_rwlock.h>
#include <event_engine.h>

/**
 * Process all the events in this thread
 * @description:
 *     null
 * @input: param;
 * @return: thread exit code
 */
unsigned LBS_THREAD_CALL ecore_event_handler(void* object, void* param1, void* param2, unsigned event_id)
{
    switch (event_id) {
        case ECORE_EVENT_POST_MESSAGE:
            {
                ECORE_MESSAGE* msg = object;
                ECORE_MODULE* mod = get_specified_module(msg->module_id);

				if (mod != 0) {
					if (mod->onmessage)
						mod->onmessage(msg->msg, msg->data);
				} else {
					printf("cannot find module(%d)\n", msg->module_id);
				}
            }
            break;
        case ECORE_EVENT_TCP_SERVER_CLIENT_RECV:
            break;
        case ECORE_EVENT_TCP_SERVER_CLIENT_DISCONNECT:
            break;
        case ECORE_EVENT_TCP_CLIENT_RECV:
            break;
        default:
            ;//printf("invalid message type, count %d\n", __event_total_handler);
    }

    return 0;
}

void init_event()
{
    lbs_status_t st;

    st = ee_init_event(ecore_event_handler);
    if (st != LBS_CODE_OK)
        ecore_sys_terminate();
}

int post_event(ECORE_MESSAGE* msg)
{
    int st;

    st = ee_post_event(msg, 0, 0, ECORE_EVENT_POST_MESSAGE);

    return st;
}


