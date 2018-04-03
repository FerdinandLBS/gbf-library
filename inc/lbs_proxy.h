#ifndef _LBS_PROXY_H
#define _LBS_PROXY_H

#include "lbs_def.h"

/* LBS proxy module error codes */
#define LBS_PROXY_ERR_BASE 1000

/* Codes */
#define LBS_PROXY_SUCCESS      0                    // Operation success
#define LBS_PROXY_INV_PARAM    1+LBS_PROXY_ERR_BASE // Invalid parameter
#define LBS_PROXY_INV_HANDLE   2+LBS_PROXY_ERR_BASE // Invalid handle value
#define LBS_PROXY_OUT_OF_MEM   3+LBS_PROXY_ERR_BASE // There is not enough memroy
#define LBS_PROXY_NO_CALLBACK  4+LBS_PROXY_ERR_BASE // There is not callback function
#define LBS_PROXY_NOT_READY    5+LBS_PROXY_ERR_BASE // The server is not ready

/* Data receive call back function should return these code for command */
#define LBS_PROXY_OK        0 // Do nothing
#define LBS_PROXY_ECHO      1 // Echo back to sender
#define LBS_PROXY_KICK_OFF  2 // Tick this sender off
#define LBS_PROXY_SEND_TO   3 // Send data to specified socket
#define LBS_PROXY_FEED_BACK 4 // Send data to sender
#define LBS_PROXY_DISPLAY   5 // Display message to console


/* Proxy status */
#define LBS_PROXY_UNINIT 0x00000000
#define LBS_PROXY_LOCKED 0X00000001
#define LBS_PROXY_READY  0x10000000

/* Handle of lbs_proxy */
typedef unsigned int lbs_proxy_handle_t;

typedef struct _lbs_proxy_send_to
{
    int *pClientArray;
    lbs_u32_t ClientCount;
    lbs_u8_t *pData;
    lbs_u32_t DataLen;
}lbs_proxy_send_t;

typedef lbs_u32_t(*do_receive_cb)(lbs_u8_t *pData, lbs_u32_t Len, int Socket, lbs_u8_t **pOutput, lbs_u32_t *pOutLen);
typedef lbs_u32_t(*do_accept_cb)(int Socket);

#ifdef __cplusplus
extern "C" {
#endif

    /* [Initialize proxy]
    ** Always call this function before you 
    ** start server.
    ** [IN] Port number
    ** [IN] Counts of worker thread
    ** [OUT]Pointer of proxy handle 
    ** Return 0 if success
    */
    lbs_u32_t lbs_proxy_init(
        lbs_u16_t Port,   /* port number */
        lbs_u8_t nThreads, /* threads of proxy */
        lbs_proxy_handle_t *hProxy
        );

    /* [Set callback function]
    ** Set callback function of proxy.
    ** [IN] Least time of client heart beat
    ** [IN] Timeout of client
    ** [IN] Pointer of receive callback function
    ** [IN] Pointer of accept callback function
    ** Return 0 if success
    */
    lbs_u32_t lbs_proxy_set_callback(
        lbs_proxy_handle_t hProxy,
        lbs_u32_t ClientTimeout,
        do_receive_cb xRecvCb,
        do_accept_cb xAcceptCb
        );

    /* [Start proxy and listen port]
    ** This function will start server and listen specified port.
    ** Caller should know that this function will always success.
    ** Nothing return if server crashes.
    */
    lbs_u32_t lbs_proxy_start(lbs_proxy_handle_t hProxy);

    /* [Close proxy and free memory]
    ** This function will close the server and free all the memroy.
    ** Caller should init a new proxy to restart.
    ** [IN] Handle of proxy
    ** Return 0 if success
    */
    lbs_u32_t lbs_proxy_close(lbs_proxy_handle_t hProxy);

#ifdef __cplusplus
};
#endif

#endif


