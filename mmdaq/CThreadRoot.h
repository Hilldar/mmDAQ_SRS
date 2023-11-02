/*
 *  CThreadRoot.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/10/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */


#ifndef CThreadRoot_h
#define CThreadRoot_h
#include <TThread.h>

//#include <pthread.h>
#include <string>
#include <iostream>

class CThreadRoot {
	
public:
	CThreadRoot(): m_thread_id(0), m_root_thread(0), m_thread_exit_code(0), m_thread_name("thread") { };
	virtual ~CThreadRoot() { delete m_root_thread; m_root_thread = 0;};
	int start_thread();
	void join_thread();
	int cancel_thread();
	void test_cancel_thread();
	void set_cancel_state_on() { TThread::SetCancelOn();};
	void thread_cancel_point() { TThread::CancelPoint();};
	
//	virtual void main() = 0;
	virtual int execute_root_thread() = 0; //Implement this method in your subclass with the code you want your thread to run.
	//virtual int cleanup_on_exit() { return 0; };	// use with pthread_cleanup_push()
	int exit_thread_code() { return m_thread_exit_code; }
	void set_thread_name(const char* name) {m_thread_name = name;}

protected:
	int m_thread_id;
	TThread* m_root_thread;
	
	//pthread_t m_thread;
	int m_thread_exit_code;
	std::string m_thread_name;
	
	
	void lock_mutex() const;
	void unlock_mutex() const;
	void signal_condition(TCondition *cond);
	void wait_condition(TCondition *cond);
	void wait_condition_timed(TCondition *cond,int sec, int nsec);

	
	//void attach_log();
private:
	CThreadRoot( const CThreadRoot&);
	CThreadRoot& operator=(const CThreadRoot&);
	static void* thread_router(void* arg)
	{
		reinterpret_cast<CThreadRoot*>(arg)->exec_thread();
		return NULL;

	};
	void exec_thread() { m_thread_exit_code = execute_root_thread(); }
	
	
	
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






