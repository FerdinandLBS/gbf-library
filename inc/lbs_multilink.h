/************************************************************************
* (C) Ferdinand Yang Copyright 2015
************************************************************************/
/*! \file: lbs_multilink.h
*	\brief
*	header of lbs_multilink.c
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-09-1
*/

#ifndef _LBS_MULTILINK_H_ 
#define _LBS_MULTILINK_H_ 

#define MLINK_E_SUCCESS			0
#define MLINK_E_INVALID_PARA	1
#define MLINK_E_BAD_MEM			2

#include <WinDef.h>

typedef struct _multi_link
{
	void*				pData;
	UINT				szData;
	char				key[32];

	struct _multi_link* pChildA; 
	struct _multi_link* pChildB;
	struct _multi_link* pNext;
	struct _multi_link* pPrev;

}MULTILINK, *pMULTILINK;



#ifdef __cplusplus 
extern "C" { 
#endif 

pMULTILINK InitMultiLinkHead(char* key, void* pData, UINT szData);





#ifdef __cplusplus 
}; 
#endif 


#endif //_LBS_MULTILINK_H_

