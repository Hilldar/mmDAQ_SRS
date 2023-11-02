/*
 *  CReceiver.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/26/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CReceiver_h
#define CReceiver_h

#include "CThread.h"
#include "CMutex.h"

#include <utility>
#include <list>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>


class CConfiguration;
class CUDPData;

class CReceiver : public CThread {
public:
	CReceiver(CConfiguration* config, std::list<CUDPData*>* dataqueue, CMutex* recv_data_mutex, pthread_cond_t& cond);
	~CReceiver();
	
	int init();
	int main(void*);
	int finish() { return close(m_socket); };
	const std::string fail_msg() const { return m_fail_message;}
	virtual int execute_thread() { return main((void*)0); }; 

	void event_ready_set()		{ m_event_ready |= 1;} 
	void event_ready_unset()	{ m_event_ready = 0;} 
	bool event_ready_check()	{ return m_event_ready;}
	
	inline void endian_swap(unsigned short& x)
	{
		x = (x>>8) | (x<<8);
	}
	
	inline void endian_swap(unsigned int& x)
	{
		x = (x>>24) | 
		((x<<8) & 0x00FF0000) |
		((x>>8) & 0x0000FF00) |
		(x<<24);
	}
	

private:
	//std::list< std::pair<uint32_t*, long> >* m_datacontainer; // char buffer , num of ints
	/*pthread_mutex_t* m_action_mutex;*/

	
	CConfiguration*				m_config;
	std::list <CUDPData*>*		m_datacontainer;
	CMutex*							m_recv_mutex;
	//pthread_mutex_t*			m_data_mutex;
	pthread_cond_t*				m_action_event_received;
	int								m_socket;
	
	std::string						m_daq_ip_address;
	std::string						m_daq_ip_port;
	std::vector< std::string>	m_fec_ip_address;
	
	bool								m_event_ready;
	std::string						m_fail_message;
	bool								m_pause_run;
	bool								m_pause_run_requested;
   size_t                     m_ignored_events_counter;
   size_t                     m_received_events_counter;
   
public:
	size_t m_internal_queue_size;
	const size_t internal_queue_size() const { return m_internal_queue_size; }
	void request_pause_run(bool state = true) { m_pause_run_requested = state; }
	size_t get_ignored_events_count() const { return m_ignored_events_counter;}
   size_t get_received_events_count() const { return m_received_events_counter;}
private:
	CReceiver(const CReceiver&);
	CReceiver operator=(const CReceiver&);
	
	void *get_in_addr(struct sockaddr *sa);
	char * get_ip_str(const struct sockaddr *sa, char *s, socklen_t maxlen);
	
};


#endif

