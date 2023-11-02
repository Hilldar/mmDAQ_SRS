/*
 *  CUDPData.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/2/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CUDPData.h"
//#include <iomanip>
//#include <iostream>


CUDPData::CUDPData(unsigned char *buffer, long numbytes, const char* ip, struct timeval* timeval, unsigned long frame_counter, size_t event_number)
//CUDPData::CUDPData(uint32_t *data32, long int32size, const char* ip, struct timeval* timeval)
: m_id(frame_counter),// m_time(0), 
m_timeval(*timeval), m_ip_address(ip), m_int32size(numbytes/MMDAQ_INT32_SIZE), m_vectordata(m_int32size,0), m_event_number(event_number)
{	
	uint32_t* iptr = reinterpret_cast<uint32_t*> (buffer);
	for (int i = 0; i < m_int32size; ++i) {
//		m_vectordata[i] = *(iptr+i); //
      m_vectordata[i] = endian_swap32_copy( *(iptr+i) ); //TODO: depends on HW equipment APZ/ADC
	}
}



CUDPData::~CUDPData() 
{
//	delete[] m_data32;
//	m_data32 = 0;
}

const std::vector<uint32_t> & CUDPData:: vectored_data() const
{
//	for (long i = 0; i < m_int32size; ++i) {
//		m_vectordata[i] =  endian_swap32_copy( *(m_data32+i) );
//		//endian_swap32_copy(m_vectordata[i] );
//	}

	return m_vectordata;
}


