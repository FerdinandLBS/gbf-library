/************************************************************************
* (C) Copyright 200X
************************************************************************/
/*! \file: 
*	\brief
*
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-07-22
*/

#ifndef _LBS_STRING_H_
#define _LBS_STRING_H_

#include <stdlib.h>
#include <string.h>
#include "lbs_def.h"

#ifdef __cplusplus
extern "C" {
#endif	


	//-------------------------------------------------------
	// Replace strings matched "sMatchStr" with "sReplaceStr"
	//-------------------------------------------------------
	int strreplace(_Inout_ char *sSrc, _In_ char *sMatchStr, _In_ char *sReplaceStr);


	//-------------------------------------------------------
	// This function will allocate memories for Strs[].
	// Please free them after used.
	//-------------------------------------------------------
	int strtostrs(_In_ char* sSrc, _In_ char* flag, _Out_ char* Strs[]);


	//-------------------------------------------------------
	// Change letters in source string to upper or lower
	// iLen controls the length to be check.
	// If iLen is set to -1 or 0, all letters will be checked.
	//-------------------------------------------------------
	int strtoupper(_In_ char* sSrc, _In_ int iLen);
	int strtolower(_In_ char* sSrc, _In_ int iLen);

    lbs_u32_t lbs_strcpy(char* s1, const char* s2);

#ifdef __cplusplus
};
#endif


#endif