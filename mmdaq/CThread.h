/*
 *  CThread.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/26/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CThread_h
#define CThread_h

#include <pthread.h>
#include <string>
#include <iostream>
#include <map>


class CThread {
	
public:
	CThread();
	virtual ~CThread();
	int start_thread();
	void join_thread();
	int cancel_thread();
	void test_cancel_thread();
	
	virtual int execute_thread() = 0; //Implement this method in your subclass with the code you want your thread to run.
	virtual int cleanup_on_exit() { return 0; };	// use with pthread_cleanup_push()
	int exit_thread_code() { return m_thread_exit_code; }
	void set_thread_name(const char* name) {m_thread_name = name;}
	const std::string thread_name() const { return m_thread_name;}
	const pthread_t thread_id() const { return m_thread;}
	
protected:
	int m_thread_id;
	pthread_t m_thread;
	int m_thread_exit_code;
	std::string m_thread_name;

	int lock_mutex(pthread_mutex_t* mutex );
	int unlock_mutex(pthread_mutex_t *mutex );
	void signal_condition(pthread_cond_t *cond);
	
	//void attach_log();
private:
	CThread(const CThread& rhs);
	CThread& operator=(const CThread& rhs);
	
	static void* thread_router(void* arg)
	{
		reinterpret_cast<CThread*>(arg)->exec_thread();
		return NULL;
	};
	void exec_thread() { m_thread_exit_code = execute_thread(); }
		
};

#endif


//class MyThreadClass
//{
//public:
////   MyThreadClass() {/* empty */}
////   virtual ~MyThreadClass() {/* empty */}
//	
////   /** Returns true if the thread was successfully started, false if there was an error starting the thread */
////   bool StartInternalThread()
////   {
////      return (pthread_create(&_thread, NULL, InternalThreadEntryFunc, this) == 0);
////   }
//	
////   /** Will not return until the internal thread has exited. */
////   void WaitForInternalThreadToExit()
////   {
////      (void) pthread_join(_thread, NULL);
////   }
//	
//protected:
//   /** Implement this method in your subclass with the code you want your thread to run. */
//   //virtual void InternalThreadEntry() = 0;
//	
//private:
//   //static void * InternalThreadEntryFunc(void * This) {((MyThreadClass *)This)->InternalThreadEntry(); return NULL;}
//	
////   pthread_t _thread;
//};


// Obviously not complete, but this should demonstrate the basic idea....
//class thread
//{
//public:
//	thread() : code_(0) { }
//	void start() { pthread_create(&pth_, NULL, thread::thread_router, reinterpret_cast<void*>(this)); }
//	void join() { pthread_join(&pth_, NULL); }
//	
//	virtual int exec() = 0;
//	int exit_code() { return code_; }
//	
//private:
//	static void* thread_router(void* arg);
//	//void exec_thread() { code_ = exec(); }
//	pthread_t pth_;
//	int code_;
//};
//void*
//thread::thread_router(void* arg)
//{
//	reinterpret_cast<thread*>(arg)->exec_thread();
//	return NULL;
//}


//class my_thread : public thread
//{
//public:
//	my_thread(int arg1, int arg2) : arg1_(arg1), arg2_(arg2) { }
//	virtual int exec();
//	
//private:
//	int arg1_, arg2_;
//};
//
//int
//my_thread::exec()
//{
//	// use arg1_, arg2_....
//	return 0;
//}
//
//
//my_thread thr(1, 2);
//thr.start();
//
//// Wait for it to complete...
//thr.join();






