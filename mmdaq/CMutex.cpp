/*
 *  CMutex.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/14/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CMutex.h"
#include "CThread.h" 
#include "ProgException.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <stdio.h> //perror



CMutex::CMutex()
: m_locked_status(0), m_locking_thread_name(), m_locking_thread_id(0)
{
	int rc = pthread_mutex_init(&m_mutex, NULL);
	if (rc) {
		throw  CProgException("CMutex::Error pthread_mutex_init(m_publishing_mutex)");
	}
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	rc = pthread_create(&m_invalid_thread, NULL, thread_router_dummy, reinterpret_cast<void*>(this));
	 
}


CMutex::~CMutex()
{
//	assert(m_locked_status == false);

	int rc = pthread_mutex_destroy(&m_mutex);
   if (rc) { 
      /* handle this case here... */
      perror("CMutex::~CMutex() pthread_mutex_destroy()");
      if (rc == EBUSY) {
			std::cerr << "CMutex::~CMutex() The mutex is currently owned by thread: "<< m_locking_thread_name << std::endl;
      }
      else if (rc == EINVAL) {
			std::cerr << "CMutex::~CMutex()  The value specified for the argument is not correct." << std::endl;
      }
   }
}


//void CMutex::thread_router_dummy() {};

int CMutex::lock()
{
	//std::cout << "CMutex::lock(): for: " << name << " in " << (callername? callername:"?") << std::endl;
	
	
	int rc = pthread_mutex_trylock(&m_mutex);
   
	if (rc) { /* an error has occurred */
		std::stringstream ss;
		ss << "Error:  CMutex::lock(): status: " << rc << "for mutex " << &m_mutex;
		perror("pthread_mutex_unlock");
		throw CProgException(ss.str());
	}
	//	if (!threadname.empty()) {
	//		m_locking_thread_name = threadname;
	//	}
//	m_locking_thread_id = thread_id;
	m_locked_status = true;
	return rc;
}


int CMutex::lock(pthread_t thread_id, std::string name, const char* callername)
{
//	std::cout << "CMutex::lock(): for: " << name << " in " << (callername? callername:"?") << std::endl;

	
//	assert(!pthread_equal(pthread_self(), m_locking_thread_id ) );
	if (pthread_equal(pthread_self(), m_locking_thread_id )) {
		std::cout << "CMutex::lock(pthread_t thread_id): equal threads id for: " << name << " in " << (callername? callername:"?") << std::endl;
	}
	int rc = pthread_mutex_lock(&m_mutex);
	if (rc) { /* an error has occurred */
		std::stringstream ss;
		ss << "Error:  CMutex::lock(): status: " << rc << "for mutex " << &m_mutex;
		perror("pthread_mutex_unlock");
		throw CProgException(ss.str());

	}
//	if (!threadname.empty()) {
//		m_locking_thread_name = threadname;
//	}
	m_locking_thread_id = thread_id;
	m_locked_status = true;
	return rc;
}


int CMutex::unlock()
{
	//std::cout << "CMutex::unlock(): for: " << m_locking_thread_name << std::endl;

	m_locking_thread_name.clear();
	m_locked_status = false;
	m_locking_thread_id = m_invalid_thread;
//	assert(m_locked_status == true);
	int rc = pthread_mutex_unlock(&m_mutex);
	if (rc) { /* an error has occurred */
		std::stringstream ss;
		ss << "Error:  CMutex::unlock(): status: " << rc << "for mutex " << &m_mutex ;
		perror("pthread_mutex_unlock");
		throw CProgException(ss.str());
	}

	return rc;
}


bool CMutex::check_lock()
{
	return m_locked_status;
}


std::string CMutex::check_locking_name()
{
	return m_locking_thread_name;
}


int CMutex::timedwait(pthread_cond_t *cond,
							 const struct timespec *timeout, 
							 std::string threadname)
{
//	assert(m_locked_status == true);
	/* remember that pthread_cond_timedwait() unlocks the mutex on entrance */
//	m_locking_thread_name.clear();
//	m_locked_status = false;
	int rc = pthread_cond_timedwait(cond, &m_mutex, timeout);
//	m_locking_thread_name = threadname;
//	m_locked_status = true;	
	return rc;

	
}

