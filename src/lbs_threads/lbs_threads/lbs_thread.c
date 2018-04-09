#include "lbs_thread.h"

int lbs_thread_get_sys_error()
{
#ifdef _WIN32
    return GetLastError();
#else
#endif
}

lbs_status_t lbs_thread_error_code_convert(int code)
{
    switch (code) {
#ifdef _WIN32
        case NO_ERROR:
            return LBS_CODE_OK;
        case 1:
            //return LBS_CODE_INV_PARAM;
        default:
            return code;
    }
#else
        case 1:
            return LBS_CODE_INV_PARAM;
        default:
            return rc;
    }
#endif
}

lbs_status_t lbs_create_thread(
    void* param, 
    lbs_thread_func func, 
    int flag,
    lbs_thread_t* thread
    )
{
#ifdef _WIN32
    int local_flag = 0;
#endif
    if (func == 0 || thread == 0)
        return LBS_CODE_INV_PARAM;

#ifdef _WIN32
    if (flag & LBS_THREAD_CREATE_SUSPENDED) {
        local_flag |= CREATE_SUSPENDED;
    }
    *thread = (HANDLE)_beginthreadex(0, 0, func, param, local_flag, 0);
    if (*thread == 0) {
        int rc = lbs_thread_get_sys_error();
        return lbs_thread_error_code_convert(rc);
    }

#else
    *thread = pthread_create();

#endif

    return LBS_CODE_OK;
}

lbs_status_t lbs_suspend_thread(lbs_thread_t thread)
{
    lbs_status_t result;

#ifdef _WIN32
    DWORD rc = SuspendThread(thread);

    if (rc == -1)
        result = lbs_thread_error_code_convert(rc);
    else
        result = LBS_CODE_OK;
#else

    result = LBS_CODE_OK;
#endif

    return result;
}

lbs_status_t lbs_resume_thread(lbs_thread_t thread)
{
    lbs_status_t result;

#ifdef _WIN32
    DWORD rc = ResumeThread(thread);
    
    if (rc == -1)
        result = lbs_thread_error_code_convert(rc);
    else
        result = LBS_CODE_OK;
#else
    result = LBS_CODE_OK;
#endif

    return result;
}

lbs_status_t lbs_terminate_thread(
    lbs_thread_t thread,
    int exit_code
    )
{
#ifdef _WIN32
    DWORD rc = NO_ERROR;
    BOOL res;

    res = TerminateThread(thread, exit_code);
    if (res != TRUE) {
        rc = GetLastError();
        return lbs_thread_error_code_convert(rc);
    }
    CloseHandle(thread);
#else
#endif

    return LBS_CODE_OK;
}

lbs_status_t lbs_wait_and_terminate_thread(
    lbs_thread_t thread, 
    int exit_code
    )
{
    lbs_status_t rc;
#ifdef _WIN32
    rc = lbs_terminate_thread(thread, exit_code);
    if (rc != LBS_CODE_OK)
        return rc;

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
#endif

    return rc;
}


lbs_status_t lbs_wait_thread(
    lbs_thread_t thread,
    int time_limit
    )
{
    lbs_status_t rc = LBS_CODE_OK;
#ifdef _WIN32
    DWORD code;

    code = WaitForSingleObject(thread, time_limit);
    if (code != NO_ERROR) {
        if (code == WAIT_TIMEOUT)
            rc = LBS_CODE_TIME_OUT;
        else
            rc = LBS_CODE_INTERNAL_ERR;
    }

    CloseHandle(thread);
#else
#endif

    return rc;
}
