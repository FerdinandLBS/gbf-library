/************************************************************************
* (C) Copyright 2015
************************************************************************/
/*! \file: lbs_def.h
*	\brief
*	public defintion and varaibles
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-9-10
*/

#ifndef _LBS_DEF_H_
#define _LBS_DEF_H_

typedef signed char lbs_s8_t;
typedef unsigned char lbs_u8_t;
typedef short lbs_s16_t;
typedef unsigned short lbs_u16_t;
typedef int lbs_s32_t;
typedef unsigned int lbs_u32_t;
typedef long long lbs_s64_t;
typedef unsigned long long lbs_u64_t;
typedef int lbs_bool_t;

typedef enum _lbs_lib_status_code {

    /* [Opereation/Request success]
    ** Not an error code
    */
    LBS_E_SUCCESS = 0,

    /* [Invalid parameter(s)]
    ** Input parameters are not valid. You may miss some parameters
    */
    LBS_E_INV_PARAM	= 1,

    /* [Out of memory]
    ** There is no free memory. APP can not allocate more.
    */
    LBS_E_OUT_OF_MEMORY = 2,

    /* [Cannot create a specified file]
    ** Function cannot create a file for some reasons.
    */
    LBS_E_CREATE_FILE_FAILED = 3,

    /* [Something you want to create/insert/appedn is already exist]
    ** Try to use it not create it
    */
    LBS_E_ALREADY_EXIST = 4,

    /* [Target is not found]
    ** Something you are looking for is not found. Maybe you don't
    ** have privilege or target does not exists
    */
    LBS_E_NOT_FOUND = 5,

    /* [Invalid handle value]
    ** A handle parameter is invalid. Please check parameters.
    */
    LBS_E_INV_HANDLE = 6,

    /* [Request of access is denied]
    ** You may not have privilege or target is used by other process.
    */
    LBS_E_ACCESS_DENIED = 7,
}lbs_status_t;


#define LBS_MAX_PATH	260
#ifndef KB
#	define KB			* 1024
#endif

#ifndef MB
#	define MB			* 1024 * KB
#endif

#ifndef LBS_TRUE
#define LBS_TRUE 1
#endif

#ifndef LBS_FALSE
#define LBS_FALSE 0
#endif

#define INTERNALCALL static


#endif //_LBS_DEF_H_
