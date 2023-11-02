/*
 *  CMutex.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/14/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 *  to manage locks 
 */

#ifndef CMutex_h
#define CMutex_h

#include <pthread.h>
#include <string>

class CMutex {
	
public:
	CMutex();
	
	virtual ~CMutex();
	int lock();
	int lock(pthread_t thread_id, std::string name = std::string(), const char* callername = 0 );
	int unlock();
	bool check_lock();
	std::string check_locking_name();
//	void set_locking_name(std::string name) { m_locking_thread_name = name;}
//	void clear_locking_name() { m_locking_thread_name.clear();}
	
//	void set_lock_status(bool status) { m_locked_status = status;}

	int timedwait(pthread_cond_t* cond,
					  const struct timespec * abstime, 
					  std::string threadname = std::string());
	
protected:
	pthread_mutex_t m_mutex ;
	bool m_locked_status;
	
	std::string m_locking_thread_name;
	pthread_t m_locking_thread_id;
	pthread_t m_invalid_thread;
	
private:
	CMutex(const CMutex&);
	CMutex& operator=(const CMutex&);
	
	static void* thread_router_dummy(void* )
	{
		//reinterpret_cast<CThread*>(arg)->exec_thread();
		return NULL;
	};
	

};



#endif

