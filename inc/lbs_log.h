/************************************************************************
* LBS Yang(C) Copyright 2015-2025. All rights reserved.
************************************************************************/
/*! \file: logfunc.h
*	\brief
*	Header of log_lib.c
*	\author Ferdinand Yang(LBS)
*	\version 1.0
*	\date 2015-07-30
*/

#ifndef _LOG_LIB_ 
#define _LOG_LIB_


#define MAX_LOGSIZE				100*1024*1024

/* Log level */
#define LOG_DEBUG				 8
#define LOG_WARN				 9
#define LOG_INFO				 10
#define LOG_MAIN				 11
/* Log level */

//+--------------------------------------------------
//| type define
//+--------------------------------------------------
#ifndef _FILE_DEFINED
struct _iobuf {
	char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
};
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif



typedef struct _lbs_log_info
{
	FILE*			pFile;
	unsigned long	FileSize;
	unsigned long	FilePos;
	char			FileName[260];
}FILEHANDLE, *pFILEHANDLE;

typedef unsigned long FSIZE;


#ifdef __cplusplus 
extern "C" { 
#endif 

	//+--------------------------------------------------
	//| Open a log file and format it
	//|
	//| FormatName: The name of log file, can't be NULL
	//|             the file must exist
	//| Return    : Handle of log file if success
	//+--------------------------------------------------
	pFILEHANDLE InitLogFile(int iPrivilege, char* FormatName,...);

	//+--------------------------------------------------
	//| Create a log file 
	//|
	//| iPrivilege: Privilege of log.
	//| FormatName: The name of log file, can't be NULL
	//|             the file must exist
	//| Return    : Handle of log file if success
	//+--------------------------------------------------
	int CreateLogFile(int iPrivilege, char* FormatName,...);

	//+--------------------------------------------------
	//| Set privilege for a log file 
	//|
	//| FileHandle   : Log file handle return from InitLogFile
	//| iNewPrivilege: New privilege going to set
	//| Return       : Zero if success
	//+--------------------------------------------------
	int SetLogPrivilege(pFILEHANDLE FileHandle, int iNewPrivilege);

	//+--------------------------------------------------
	//| Write a log to log file
	//|
	//| FileHandle: Log file handle return from InitLogFile
	//| iPrivilege: New privilege going to set
	//| FormatMsg : Context of log 
	//| Return    : Zero if success
	//+--------------------------------------------------
	int WriteLogFile(pFILEHANDLE FileHandle, int iPrivilege, char* FormatMsg,...);

	//+--------------------------------------------------
	//| Read context of a log into buffer
	//|
	//| FileHandle: Log file handle return from InitLogFile
	//| pBuf      : Buffer to store context of log
	//|             Use GetLogSize() to alloc memory for 
	//|	            buffer.
	//| Return    : Zero if success
	//+--------------------------------------------------
	int ReadLogFile(pFILEHANDLE FileHandle, char* pBuf);

	//+--------------------------------------------------
	//| Delete a log file
	//|
	//| cFileName: Name of the file you want to delete.
	//| Return   : Zero if success
	//+--------------------------------------------------
	int DeleteLogFile(char* strFileName);

	//+--------------------------------------------------
	//| Get size of a log file
	//|
	//| strFileName: Name of the file.
	//| Return     : Size of the log file if success.
	//+--------------------------------------------------
	FSIZE GetLogSize(char* strFileName);

	//+--------------------------------------------------
	//| Close a log file when you will not use it any more
	//|
	//| FileHandle: Log file handle return from InitLogFile.
	//| Return    : Zero if success.
	//+--------------------------------------------------
	int CloseLogFile(pFILEHANDLE FileHandle);

#ifdef __cplusplus 
}; 
#endif 


#endif //_LOG_LIB_

