#ifndef _LBS_H_
#define _LBS_H_

#define LBS_CALLCONV _stdcall

typedef unsigned char  lbs_byte_t;
typedef unsigned short lbs_word_t;
typedef unsigned int   lbs_dword_t;
typedef unsigned int   lbs_size_t;
typedef unsigned int   lbs_status_t;

/*
 * Error codes
 * Start from zero. Zero means OK
 */
#define LBS_CODE_OK            0 // Process successfully 
#define LBS_CODE_INV_PARAM     1 // An invalid parameter is inputed
#define LBS_CODE_NOT_FOUND     2 // Specified item cannot be found
#define LBS_CODE_NO_MEMORY     3 // This process is out of memory
#define LBS_CODE_ALREADY_EXIST 4 // Same item already exists, cannot create more
#define LBS_CODE_DENIED        5
#define LBS_CODE_INV_HANDLE    6
#define LBS_CODE_INV_FLAG      7
#define LBS_CODE_HANDLE_TYPE_MISMATCH 8
#define LBS_CODE_TIME_OUT      9
#define LBS_CODE_DATA_CORRUPT  10

// BST
#define LBS_CODE_INV_KEY    305
#define LBS_CODE_EMPTY_TREE 306

/* Net */
#define LBS_CODE_SENDING_UNCOMPLETE  1001 // Not all datas are send
#define LBS_CODE_SOCKET_UNWRITABLE   1002 // The socket is not writable
#define LBS_CODE_SERVER_TERMINATED   1003 // The server is closed
#define LBS_CODE_CLIENT_DISCONNECTED 1004
#define LBS_CODE_FAILED_TO_BIND      1005
#define LBS_CODE_FAILED_TO_CONNECT   1006
#define LBS_CODE_TOO_MANY_SOCKETS    1007
#define LBS_CODE_BUFFER_OVERFLOW     1008 // Buffer is too small
#define LBS_CODE_NETWORK_ISSUE       1009
#define LBS_CODE_REMOTE_DISCONNECT   1010
#define LBS_CODE_REMOTE_REFUESD      1011
#define LBS_CODE_BUFFER_TOO_LARGE    1012

#define LBS_CODE_UNCOMPLETE_PACKET   1013
#define LBS_CODE_MULTIPLE_PACKET     1014

/* Thread pool */
#define LBS_CODE_WATCHER_NOT_READY 3001
#define LBS_CODE_WATCHER_EXIST     3002
#define LBS_CODE_SUSPEND_BY_OTHERS 3003
#define LBS_CODE_NO_TASK           3004


#define LBS_CODE_INTERNAL_ERR -1
#define LBS_CODE_WIN_ERROR    -2
#define LBS_CODE_SOCK_ERROR   -3

#endif // End of _LBS_ERRNO_H_

