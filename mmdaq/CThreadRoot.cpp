/*
 *  CThreadRoot.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/10/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 *  http://www-linux.gsi.de/~go4/HOWTOthreads/howtoframe.html
 */


#include "CThreadRoot.h"

#include <TCondition.h>

#include <errno.h>
#include <iostream>

using std::cout;
using std::endl;
using std::cerr;



int CThreadRoot::start_thread()
{ 
	if (m_root_thread) {
		return 1;
	}
	m_root_thread = new TThread(m_thread_name.c_str(), (void(*) (void *))&thread_router, (void*) this);
	m_root_thread->Run();
	return 0;
}

void CThreadRoot::join_thread() 
{
	if (!m_root_thread) {
		return;
	}
	m_root_thread->Join();
}

int CThreadRoot::cancel_thread() 
{ 
	if (!m_root_thread) {
		return 1;
	}
	TThread::Delete(m_root_thread);
	delete m_root_thread;
	m_root_thread = 0;
	return 0;
	
}

void CThreadRoot::test_cancel_thread()
{
	//pthread_testcancel();
}


void CThreadRoot::lock_mutex() const
{
	//cout << "CThreadRoot::  lock_mutex() @" << m_thread_name;
	TThread::Lock();
	//cout << "   locked " << m_thread_name << endl;
}

void CThreadRoot::unlock_mutex() const
{
	//cout << "CThreadRoot::unlock_mutex()_@" << m_thread_name;
	TThread::UnLock();
	//cout << " unlocked_@" << m_thread_name << endl;

}



/*
 # TCondition::Wait() waits until any thread sends a signal of the same condition instance: 
 MyCondition.Wait() reacts on MyCondition.Signal() or MyCondition.Broadcast(); MyOtherCondition.Signal() has no effect.
 # If several threads wait for the signal of the same TCondition MyCondition, 
 at MyCondition.Signal() only one thread will react; 
 to activate a further thread another MyCondition.Signal() is required, etc.
 # If several threads wait for the signal of the same TCondition MyCondition, 
 at MyCondition.Broadcast() all threads waiting for MyCondition are activated at once. 
 Remark: In some tests of MyCondition using an internal mutex, Broadcast() 
 activated only one thread (probably depending whether MyCondition had been signalled before).
 */

void CThreadRoot::signal_condition(TCondition *cond)
{
	cond->Signal();
}

void CThreadRoot::wait_condition(TCondition *cond)
{
	cond->Wait();
}


/*
 # MyCondition.TimedWait(secs,nanosecs) waits for MyCondition until the absolute time in seconds and nanoseconds 
 since beginning of the epoch (January, 1st,1970) is reached; to use relative timeouts ``delta'', 
 it is required to calculate the absolute time at the beginning of waiting ``now''; a possible coding could look like this:
 
 Ulong_t now,then,delta;              // seconds 
 TDatime myTime;                      // root daytime class 
 myTime.Set();                        // myTime set to ``now'' 
 now=myTime.Convert();                // to seconds since 1970 
 then=now+delta;                      // absolute timeout 
 wait=MyCondition.TimedWait(then,0);  // waiting
 
 Return value wait of MyCondition.TimedWait should be 0, if
 MyCondition.Signal() was received, and should be nonzero, if timeout was reached.
 */
void CThreadRoot::wait_condition_timed(TCondition *cond,int sec, int nsec)
{
	cond->TimedWait(sec,nsec);
}
