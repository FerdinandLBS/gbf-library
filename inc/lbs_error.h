/************************************************************************
* LBS Yang (C) Copyright 2015-2025 All rights reserved.
************************************************************************/
/*! \file: lbs_error.h 
*	\brief
*	Header of lbs_error.c
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2016-2-28
*/

#ifndef _LBS_ERROR_
#define _LBS_ERROR_

#ifdef _DEBUG
#define lbs_assert(_expression) if(!(_expression)) _asm {int 3}
#else
#define lbs_assert(_expression)
#endif

#define lbs_is_failed(_code) (_code != 0)
#define lbs_is_inv_handle(_handle) (_handle == 0 || _handle == (lbs_u32_t)-1)


// API
#ifdef __cplusplus 
extern "C" { 
#endif 

#ifdef __cplusplus
};
#endif

#endif
