/************************************************************************
* (C) Copyright 2015
************************************************************************/
/*! \file: lbs_threads.h
*	\brief
*	header of threads handle functions
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-09-09
*/

#ifndef _LBS_THREADS_H_ 
#define _LBS_THREADS_H_ 


unsigned long g_ThreadError;


#ifdef __cplusplus 
extern "C" { 
#endif 

	//+------------------------------------------------------
	//| Allocate a period of memory in heap.
	//|	MemorySize should be greater than 0
	//| flag: 
	//|		MEM_ORIN(0): Do nothing
	//|		MEM_CLEAN(1): Clean allocated memory
	//+------------------------------------------------------
	HANDLE lbs_newthread(size_t StackSize, LPTHREAD_START_ROUTINE lpStartAddress, void* Para, DWORD* threadid);


#ifdef __cplusplus 
}; 
#endif 


#endif //_LBS_THREADS_H_
