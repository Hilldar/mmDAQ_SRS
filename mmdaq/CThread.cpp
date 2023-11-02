/*
 *  CThread.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/26/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CThread.h"
#include <errno.h>
#include <stdio.h> //perror
#include <iostream>
#include <utility>

using std::cout;
using std::endl;
using std::cerr;


CThread::CThread()
: m_thread_id(0), m_thread(0), m_thread_exit_code(0), m_thread_name("") 
{ };


CThread::~CThread() 
{ };



int CThread::start_thread()
{ 
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int rc = pthread_create(&m_thread, NULL, CThread::thread_router, reinterpret_cast<void*>(this));
	if (rc) {
		std::cerr << "Error:  CThread::start_thread(): " << m_thread_name << "," << m_thread << " status: " << rc << std::endl; 
	}
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);	
	//m_thread_id = pthread_self();
	return rc;  
}

void CThread::join_thread() 
{
	int rc = pthread_join(m_thread, NULL); 
	if (rc) {
		std::cerr << "Error:  CThread::join_thread(): " << m_thread_name << ","<< m_thread << " status: " << rc << std::endl; 
	}

}

int CThread::cancel_thread() 
{ 
	int rc = pthread_cancel(m_thread); 
	if (rc) {
		std::cerr << "Error:  CThread::cancel_thread(): " << m_thread_name << "," << m_thread << " status: " << rc << std::endl; 
		if( rc == EINVAL) {
			cerr  << m_thread_name << " The value specified for the argument is not correct. " << endl;
		}
		else if( rc == ESRCH) {
			cerr << m_thread_name << " No thread could be found that matched the thread ID specified." << endl;
		}
	}
	return rc;

}

void CThread::test_cancel_thread()
{
	pthread_testcancel();
}


int CThread::lock_mutex(pthread_mutex_t* mutex)
{
	//cout << "lock_mutex " << mutex << " " << m_thread_name << endl;
	int rc = pthread_mutex_lock(mutex);
	if (rc) { /* an error has occurred */
		std::cerr << "Error:  CThread::lock_mutex(): " << m_thread << " status: " << rc << std::endl; 
		perror("pthread_mutex_lock");
		pthread_exit(NULL);
	}
	return rc;
}

int CThread::unlock_mutex(pthread_mutex_t *mutex)
{
	//cout << "CThread::unlock_mutex " << mutex << " " << m_thread_name << endl;
	int rc = pthread_mutex_unlock(mutex);
	if (rc) { /* an error has occurred */
		std::cerr << "Error:  CThread::unlock_mutex(): " << m_thread << " status: " << rc << std::endl; 
		perror("pthread_mutex_unlock");
		pthread_exit(NULL);
	}
	return rc;
}

void CThread::signal_condition(pthread_cond_t *cond)
{
	int rc = pthread_cond_signal(cond);
	if (rc) {
		std::cerr << "Error:  CThread::signal_mutex(): " << m_thread_name << "," << m_thread << " status: " << rc << std::endl; 
		perror("CThread::signal_mutex(): pthread_cond_signal");
		if( rc == EINVAL) {
			cerr  << m_thread_name << " The value cond does not refer to an initialised condition variable.  " << endl;
		}
		pthread_exit(NULL);
	}
}


