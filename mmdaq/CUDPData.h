/*
 *  CUDPData.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/2/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CUDPData_h
#define CUDPData_h

#define MMDAQ_INT32_SIZE 4
#define MMDAQ_INT16_SIZE 2

#ifndef SRS_NEXT_EVENT
#define SRS_NEXT_EVENT 0xFAFAFAFA
#endif

#include <ctime>
#include <stdint.h>
#include <string>
#include <vector>
#include <sys/time.h>

class CUDPData {
public:
	CUDPData(unsigned char *buffer, long numbytes, const char* ip, struct timeval* timeval, unsigned long frame_counter, size_t event_number);
	~CUDPData();
	
	const long					get_size() const { return m_int32size;}
	const std::string			get_ipaddress() const { return m_ip_address; }
	const std::vector<uint32_t>& vectored_data() const;
	const struct timeval* const get_timeval() const { return &m_timeval;}
   size_t get_idnumber() const { return m_id;}
	const uint32_t first_word32() const { return m_int32size ? m_vectordata[0] : 0; }
	
   bool is_fafa() const {
      if (m_int32size == 1 && m_vectordata[0] == SRS_NEXT_EVENT ) {
         return true;
      }
      return false;
   }
   
   inline static void endian_swap16(unsigned short& x)
	{
		x = (x>>8) | (x<<8);
	}
	
	inline static void endian_swap32(unsigned int& x)
	{
		x = (x>>24) | 
		((x<<8) & 0x00FF0000) |
		((x>>8) & 0x0000FF00) |
		(x<<24);
	}
   
   inline static int endian_swap16_copy(const unsigned short& x)
	{
		return (x>>8) | (x<<8);
   }

   
	unsigned static int endian_swap32_copy(const unsigned int x)
	{
		return (x>>24) | 
		((x<<8) & 0x00FF0000) |
		((x>>8) & 0x0000FF00) |
		(x<<24);
	}	
   
private:
	size_t	m_id;
	//time_t			m_time;
	struct timeval m_timeval;
	std::string		m_ip_address;
	long           m_int32size;
	std::vector<uint32_t> m_vectordata;
   size_t         m_event_number;


};

#endif

