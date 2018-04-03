/************************************************************************
* (C) Copyright 2015
************************************************************************/
/*! \file: lbs_mem.h
*	\brief
*	Header of lbs_mem.c
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-09-7
*/

#ifndef _LBS_MEM_H_ 
#define _LBS_MEM_H_ 

#include "lbs_def.h"

#define MEM_ORIN		0
#define MEM_CLEAN		1


#ifdef __cplusplus 
extern "C" { 
#endif 

	//+------------------------------------------------------
	//| Allocate a period of memory in heap.
	//|
	//|	MemorySize: Should be greater than 0
	//| flag: 
	//|		 MEM_ORIN(0): Do nothing
	//|		 MEM_CLEAN(1): Clean allocated memory
	//|	Return: Address of memory if success. NULL if failed
	//+------------------------------------------------------
	void* lbs_mem_alloc(lbs_u32_t size, int flag);


	//+------------------------------------------------------
	//| Free a period of memory in heap. This function will
	//| help you to set pointer to NULL
	//|
	//|	Memory: Should be None-zero
	//| MemorySize: You should better input the size of the
	//|				memory for that GetTotalMemory work well.
	//|	Return: 0 if success. -1 if failed.
	//+------------------------------------------------------
	int lbs_mem_free(void* ptr);

    int lbs_mem_cmp(lbs_u8_t* m1, lbs_u8_t* m2, lbs_u32_t size);

    void* lbs_mem_copy(void* pDst, void* pSrc, lbs_u32_t nSize);

	//+------------------------------------------------------
	//| Get memory leak if any
	//|	
	//| Return: Numbers of leak.
	//+------------------------------------------------------
	int lbs_mem_leak();

#ifdef __cplusplus 
}; 
#endif 


#endif //_LBS_MEM_H_ 

