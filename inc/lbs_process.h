/************************************************************************
* (C) Copyright 2015 Ferdinand Yang
************************************************************************/
/*! \file: lbs_process.h 
*	\brief
*	header of process handle API 
*	With this version, you can only get parent PID.
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-09-9
*/ 

#ifndef _LBS_PROCESS_H_ 
#define _LBS_PROCESS_H_ 

#define LBS_PROC_E_

typedef unsigned int lbs_pid;

#ifdef __cplusplus 
extern "C" { 
#endif 

	//+------------------------------------------------------
	//| Get parent process ID.
	//|
	//|	pid   : PID you want to check. This parameter can be 0
	//|	Return: ID of the parent process. If pid is 0 this API
	//|	        will check current module parent process ID.
	//+------------------------------------------------------
	lbs_pid ppid(lbs_pid pid);

#ifdef __cplusplus 
}; 
#endif 


#endif //_LBS_PROCESS_H_
