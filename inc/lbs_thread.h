#ifndef _LBS_THREAD_H_
#define _LBS_THREAD_H_

#include <lbs.h>

#ifdef _WIN32

#include <Windows.h>
#include <process.h>
typedef HANDLE lbs_thread_t;

#define LBS_THREAD_CALL _stdcall
#else

#include "errno.h"
typedef int enet_thread_t;

#define ENET_THREAD_CALL

#endif

typedef unsigned int (LBS_THREAD_CALL *lbs_thread_func)(void* param);

#define LBS_THREAD_CREATE_NORMAL    0x00000000
#define LBS_THREAD_CREATE_SUSPENDED 0x00000001

#ifdef __cplusplus
extern "C" {
#endif

    
    //
    lbs_status_t lbs_create_thread(
        void* param,
        lbs_thread_func func,
        int flag,
        lbs_thread_t* thread
        );

    lbs_status_t lbs_suspend_thread(
        lbs_thread_t thread
        );

    lbs_status_t lbs_resume_thread(
        lbs_thread_t thread
        );

    lbs_status_t lbs_terminate_thread(
        lbs_thread_t thread,
        int exit_code
        );

    lbs_status_t lbs_wait_and_terminate_thread(
        lbs_thread_t thread,
        int exit_code
        );

    lbs_status_t lbs_wait_thread(
        lbs_thread_t thread,
        int time_limit
        );

#ifdef __cplusplus
};
#endif


#endif//_LBS_THREAD_H_
