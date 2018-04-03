#include "enet_sys.h"

void enet_sleep(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms*1000);
#endif
}

int enet_get_sys_error(void)
{
#ifdef _WIN32
    return GetLastError();
#else
#endif
}

int enet_is_socket_disconnected(int sys_err)
{
#ifdef _WIN32
    if (sys_err == WSAECONNRESET || sys_err == WSAENOTCONN ||
        sys_err == WSAENETDOWN   || sys_err == WSAECONNABORTED)
        return 1;
#else
    return 0;
#endif

    return 0;
}

/**
 * System error code convert to local error code
 * @description:
 *     null
 * @input: system error code
 * @return: ease net library error code
 */
lbs_status_t enet_error_code_convert(int code)
{
#ifdef _WIN32
    switch (code) {
    case WSAEMFILE:
        return LBS_CODE_TOO_MANY_SOCKETS;
    case WSAEMSGSIZE:
        return LBS_CODE_BUFFER_OVERFLOW;
    case WSAENETDOWN:
    case WSAENETUNREACH:
    case WSAENETRESET:
    case WSAECONNABORTED:
        return LBS_CODE_NETWORK_ISSUE;
    case WSAECONNRESET:
        return LBS_CODE_REMOTE_DISCONNECT;
    case WSAETIMEDOUT:
        return LBS_CODE_TIME_OUT;
    case WSAECONNREFUSED:
        return LBS_CODE_REMOTE_REFUESD;
    default:
        return code;
    }
#else
#endif
}
