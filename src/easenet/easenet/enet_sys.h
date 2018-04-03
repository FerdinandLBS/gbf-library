#ifndef _ENET_SYS_H_
#define _ENET_SYS_H_

#include "lbs_errno.h"

#ifdef _WIN32
#include <Windows.h>
#else
#endif

/**
 * Sleep current thread
 * @description:
 *     null
 * @input: sleep time in million-second
 * @return: null
 */
void enet_sleep(int ms);

/**
 * Check if the error code means disconnection
 * @description:
 *     null
 * @input: system error code. Use enet_get_sys_error().
 * @return: 0 means no, otherwise yes.
 */
int enet_is_socket_disconnected(int sys_err);

/**
 * Get system last error
 * @description:
 *     null
 * @return: error code
 */
int enet_get_sys_error();

/**
 * Convert socket error code to native code
 * @description:
 *     null
 * @input: socket error code
 * @return: status
 */
lbs_status_t enet_error_code_convert(int code);


#endif // _ENET_SYS_H_

