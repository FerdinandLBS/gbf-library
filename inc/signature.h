/************************************************************************
* (C) Copyright 2015 SafeNet, Inc. All rights reserved.
*
* This software contains proprietary information, which shall not
* be reproduced or transferred to other documents and shall not
* be disclosed to others or used for manufacturing or any other
* purpose without prior written permission of SafeNet, Inc.
************************************************************************/
/*! \file: GET_SIGINFOfunc.h 
*	\brief
*	verify trust module head file
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-07-22
*/

#ifndef _GET_SIGINFO_H
#define _GET_SIGINFO_H

#define GET_SIGINFO_ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

#define GET_SIGINFO_SUCCESS									0
#define GET_SIGINFO_ERR_FILE_NOT_TRUSTED					101
#define GET_SIGINFO_ERR_FILE_UNKNOWN_PROVIDER				102
#define GET_SIGINFO_ERR_FILE_ACTION_UNKNOWN					103
#define GET_SIGINFO_ERR_FILE_SUBJECT_FORM_UNKNOWN			104
#define GET_SIGINFO_ERR_FILE_NOT_EXIST						105
#define GET_SIGINFO_ERR_FILE_BAD_DIGEST						106
#define GET_SIGINFO_ERR_FILE_UNKNOW_ERROR					109

#define GET_SIGINFO_ERR_MISMATCH_OF_PUBLICKEY				150
#define GET_SIGINFO_ERR_INVALID_PARA						151
#define GET_SIGINFO_ERR_LOADLIB_FAIL						152
#define GET_SIGINFO_ERR_NTCREATE_FAIL						153
#define GET_SIGINFO_ERR_BAD_MEMORY							155

#define GET_SIGINFO_ERR_CRYPTQUERYOBJECT_FAILED				201
#define GET_SIGINFO_ERR_CRYPTMSGGETPARAM_FAILED				202
#define GET_SIGINFO_ERR_GETNAMESTRING_FAILED				203
#define GET_SIGINFO_ERR_CERTFINDCERTIFICATEINSTORE_FAILED	204
#define GET_SIGINFO_ERR_GETPUBLICKEY_FAILED					205
#define GET_SIGINFO_ERR_DISPOSAL_FAILED						206
#define GET_SIGINFO_ERR_CRYPTDECODEOBJECT_FAILED			207
#define GET_SIGINFO_ERR_CHAIN_IS_UNCOMPLETE					208

#define GET_SIGINFO_ROOT									2
#define GET_SIGINFO_CHILD									1
#define GET_SIGINFO_PARENT									0
 

typedef struct _Trusted_White_List {
	BYTE*			pPublicKey;
	size_t			szPublicKeyLen;
	unsigned int	udMode;
}TRUSTEDWHITELIST, *pTRUSTEDWHITELIST;

#ifdef __cplusplus
	extern "C" {
#endif

int  CheckModuleSignature(
					   _In_ PCWSTR wstrFilePath, 
					   _In_ pTRUSTEDWHITELIST pWhiteList,
					   _In_ int flag);


int VerifyDigitalSign(_In_ PCWSTR wstrFilePath); 

#ifdef __cplusplus
	};
#endif

#endif //_VERIFY_TRUST_H

