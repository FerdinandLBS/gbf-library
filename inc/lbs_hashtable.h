/************************************************************************
* (C) Copyright 2015 Ferdinand Yang
************************************************************************/
/*! \file: lbs_hashtablefunc.ch
*	\brief
*	Header of lbs_hashtable.c
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-9-10
*/

//+----------------------------------------------------------------------
//| This library is used to create hash table and save data.
//| If any other lbs_libraries is linked to your project please make sure
//| that they don't include each other.
//|
//| ATTENTION:
//|   This library include lbs_mem.lib. So if you use this library DO NOT  
//| link lbs_mem.lib any more.
//+----------------------------------------------------------------------

#ifndef _LBSHASHTALBEFUNC_H_
#define _LBSHASHTALBEFUNC_H_

#include "lbs_def.h"



//+----------------------------
//| Definitions in lbs_mem.lib 
//+----------------------------
#define MEM_ORIN			0
#define MEM_CLEAN			1

#define HASH_E_SUCCESS		0
#define HASH_E_BAD_MEM		1
#define HASH_E_INVALID_KEY	2
#define HASH_E_INVALID_PARA	3
#define HASH_E_NOT_EXIST	4

#define HASH_NEW_PARENT		100
#define HASH_NEW_CHILD		101
#define HASH_NEW_VALUE		102


typedef struct _lbs_hashtable_childkey
{
	lbs_u8_t*	pData;
	lbs_u32_t	szData;
	char		key[32];
	struct _lbs_hashtable_childkey* pNext;
	struct _lbs_hashtable_childkey* pPrev;
}LBSCHILDKEY, *pLBSCHILDKEY;

typedef struct _lbs_hashtable_parentkey
{
	lbs_u8_t*			pData;
	lbs_u32_t				szData;
	char			key[32];
	pLBSCHILDKEY	pChildHead;
	pLBSCHILDKEY	pChildTail;
	struct _lbs_hashtable_parentkey* pNext;
	struct _lbs_hashtable_parentkey* pPrev;
}LBSPARENTKEY, *pLBSPARENTKEY;

typedef struct _lbs_hashtable
{
	lbs_u8_t*       pData;
	lbs_u32_t				szData;
	char			key[32];
	pLBSPARENTKEY	pParentHead;
	pLBSPARENTKEY	pParentTail;
}LBSHASHTABLE, *pLBS_TALBE;


#ifdef __cplusplus 
extern "C" { 
#endif

	//+------------------------------------------------------
	//| Create an empty hash table
	//|
	//|	key   : Can be NULL
	//| data  : Can be NULL 
	//|	szdata: Should be none-zero if data is used
	//| return: Polbs_u32_ter of hash table if success
	//+------------------------------------------------------
	pLBS_TALBE lbs_table_init(char* key, lbs_u8_t* data, lbs_u32_t size);

	//+------------------------------------------------------
	//| Add an parent link lbs_u32_to hash table. If the parent key   
	//| already exist the data will be replaced.
	//|
	//|	parentkey: Main index to be add
	//| data	 : Data that is going to be record
	//| szData	 : Size of data.
	//| hashtable: Which hash table
	//| return	 : Zero if success.
	//+------------------------------------------------------
	lbs_u32_t AddParentToHash(char* parentkey, lbs_u8_t* data, lbs_u32_t size, pLBS_TALBE hashtable);

	//+------------------------------------------------------
	//| Add an item lbs_u32_to hash table. If parent key and child 
	//| key already exist the data will be replaced.
	//|
	//|	parentkey: Main index
	//| childkey : Sub index
	//| data	 : Data that is going to be record
	//| szData	 : Size of data.
	//| hashtable: Which hash table
	//| return	 : Zero if success.
	//+------------------------------------------------------
	lbs_u32_t AddItemToHash(char* parentkey, char* childkey, lbs_u8_t* data, lbs_u32_t szData, pLBS_TALBE hashtable);

	//+------------------------------------------------------
	//| Get data from hash table.
	//|
	//|	parentkey: Main index
	//| childkey : Sub index
	//| hashtable: Which hash table
	//| return   : Polbs_u32_ter of data if success. NULL if failed.
	//+------------------------------------------------------
	lbs_u8_t* GetValueFromHash(char* parentkey, char* childkey, pLBS_TALBE hashtable);

	//+------------------------------------------------------
	//| Remove an item from a hash table
	//|
	//|	parentkey: Main index
	//| childkey : Sub index
	//| hashtable: Which hash table
	//| return   : Zero if success.
	//+------------------------------------------------------
	lbs_u32_t RemoveItemFromHash(char* parentkey, char* childkey, pLBS_TALBE hashtable);	

	//+------------------------------------------------------
	//| Remove a hashtable and free memories
	//|
	//|	HashTable: Target to remove.
	//| return   : Zero if success.
	//+------------------------------------------------------
	lbs_u32_t RemoveHashTable(pLBS_TALBE HashTable);



#ifdef __cplusplus 
}; 
#endif


#endif




