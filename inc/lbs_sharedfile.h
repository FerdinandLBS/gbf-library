/************************************************************************
* (C) Copyright 2015
************************************************************************/
/*! \file: 
*	\brief
*
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-07-22
*/

#ifndef _LBS_SHAREDFILE_H_ 
#define _LBS_SHAREDFILE_H_ 


typedef unsigned char FPTR;


typedef struct _lbs_sharedfile_attr
{
	char	FileName[260];
	HANDLE	hFileHandle;
	DWORD	hLength;
	DWORD	lLength;
	FPTR	fptr;
}SHAREDFILE, *pSHAREDFILE;




#ifdef __cplusplus 
extern "C" { 
#endif 


int lbs_init_filehandle(pSHAREDFILE hSharedFile, char* FileName, DWORD hLength, DWORD lLength);
int lbs_create_sharedfile(pSHAREDFILE hSharedFile);
int lbs_read_sharedfile(pSHAREDFILE hSharedFile, void* Buffer, DWORD dwSize);
int lbs_write_sharedfile(pSHAREDFILE hSharedFile, void* Buffer, DWORD dwSize);


#ifdef __cplusplus 
}; 
#endif 


#endif //_LBS_SHAREDFILE_H_

