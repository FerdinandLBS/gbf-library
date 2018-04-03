/************************************************************************
* LBS Yang (C) Copyright 2015-2025 All rights reserved.
************************************************************************/
/*! \file: ConfigFunc.h 
*	\brief
*	Head file of Config.c
*	\author Ferdinand Yang
*	\version 1.1
*	\date 2015-9-11
*/

//+----------------------------------------------------------------------
//| These APIs needs support of lbs_mem.lib and lbs_hashtable.lib. These
//| libraries shuold be loaded first before you use this one.
//|
//|
//|
//+----------------------------------------------------------------------

#ifndef _CONFIG_FUNC_H_
#define _CONFIG_FUNC_H_


#include "lbs_def.h"
#include "lbs_mem.h"
#include "lbs_hashtable.h"

/* Common */
#define CONF_E_SUCCESS			0
#define CONF_E_UNKNOWN_ERR		-1
#define CONF_E_INVALID_PARA		-2
#define CONF_E_FILE_NOT_EXIST	-3
#define CONF_E_BAD_MEMORY		-4
#define CONF_E_NOT_EXIST		-5
#define CONF_E_BAD_FORMAT		-6
#define CONF_E_INVALID_SECTION  7
/* Common */
		
#define CONF_PARENT_KEY			1
#define CONF_CHILD_KEY			2
#define CONF_COMMENT			3
#define CONF_BAD_FILE			4				


typedef struct _lbs_conf_context {
    char* szFile; // file name strings
    lbs_u64_t nSize; // file size
    void* pCtx;  // pointer of context list
}conf_ctx_t;

// API
#ifdef __cplusplus 
extern "C" { 
#endif 


    /* Prepare context of configure file */
    lbs_u32_t lbs_conf_prepare(
        char* szFile,     /* pointer of file name string */
        conf_ctx_t* pCtx /* pointer of conf context */
        );

    void lbs_conf_free(conf_ctx_t* pCtx);

    void lbs_conf_print(conf_ctx_t stCtx);

#ifdef __cplusplus
};
#endif

#endif
