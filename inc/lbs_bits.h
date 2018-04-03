/************************************************************************
* (C) Ferdinand Yang Copyright 2015
************************************************************************/
/*! \file: lbs_bits.h
*	\brief
*	header of lbs_bits.c
*	\author Ferdinand Yang
*	\version 1.0
*	\date 2015-07-22
*/

#ifndef _LBS_BITS_H_
#define _LBS_BITS_H_


#ifdef __cplusplus
extern "C" {
#endif	


	//-------------------------------------------------
	// Bits must be less than 8. cDST will not change.
	//-------------------------------------------------
	char cmoverc(char cDST, int Bits);
	char cmovelc(char cDST, int Bits);


	//-------------------------------------------------
	// Bits must be less than 32. iDST will not change.
	//-------------------------------------------------
	unsigned int imoverc(unsigned int iDST, int Bits);
	unsigned int imovelc(unsigned int iDST, int Bits);

	//-------------------------------------------------
	// Change a data from Big-Endian to Little-Endian or
	// by the other side.
	//
	// lpData	: Pointer of data. Data can be integer, 
	// 		      long, char and so on.
	// bitcount	: use sizeof(DataType) to get this para.
	//-------------------------------------------------	
	void le2be(void* lpData, int bitcount);

#ifdef __cplusplus
};
#endif


#endif