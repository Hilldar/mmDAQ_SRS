/*
 *  CMMDaq.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/27/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CMMDaq.h"
#include "CUDPData.h"
#include "CConfiguration.h"
#include "CReceiver.h"
#include "CPublisher.h"

#include "CEvent.h"
#include "CApvEvent.h"
#include "CMMEvent.h"
#include "CEventDecoder.h"
//#include "CApvEventDecoder.h"
#include "CRootWriter.h"
#include "CLogger.h"
#include "CUserInterface.h"
#include "CThreadRoot.h"
#include "CMutex.h"
#include "MBtools.h"


#include <TMutex.h>
#include <TCondition.h>
#include <TApplication.h>

//#include <boost/shared_ptr.hpp>
//#include <boost/thread.hpp>
//#include <boost/asio.hpp>
//#include <boost/interprocess/ipc/message_queue.hpp>


#include <iostream>
#include <errno.h>
#include <stdio.h> //perror

//#include <pthread.h>

#define MMDAQ_CONFIG_FILE "/home/physics/2017JulTB/Configs/2017JulTB_mmdaq.config"  //"/home/physics/2016JulTB/Configs/validConfigs/TB14/TB_nov14_mmdaq.config"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
//using std::queue;
//using std::vector;

//namespace bip = boost::interprocess;


//action_mutex = PTHREAD_MUTEX_INITIALIZER;
//
//data_mutex   = PTHREAD_MUTEX_INITIALIZER;
//
//
//action_cond   = PTHREAD_COND_INITIALIZER;


CMMDaq::CMMDaq(int argc, char * argv[])
      : ready_to_exit(false), m_config(0), m_logger(0), m_receiver(0), m_decoder(0),
      m_writer(0), m_publisher(0), 
//m_io_service(),
m_gui(0),
      m_datacontainer(), m_eventcontainer(), m_run_start_time(0), m_current_rate(0),
      m_mx_received(0), m_mx_processed(0), m_mx_published(new CMutex()),
      m_condition_processed_mx(0), m_condition_processed(0),
      m_pub_condition_newrequest_mx(0), m_pub_condition_newrequest(0),
      m_pub_condition_dataready_mx(0), m_pub_condition_dataready(0),
      m_internal_queue_sizes(5, 0), m_save_data_flag(false), m_busy(false)
{

  // init();

   m_condition_processed_mx = new TMutex;
   m_condition_processed = new TCondition(m_condition_processed_mx);

   m_pub_condition_newrequest_mx = new TMutex;
   m_pub_condition_newrequest = new TCondition(m_pub_condition_newrequest_mx);
   m_pub_condition_dataready_mx = new TMutex;
   m_pub_condition_dataready = new TCondition(m_pub_condition_dataready_mx);

   //m_mx_published = new CMutex();


   try {
      m_config = new CConfiguration(argc, argv, MMDAQ_CONFIG_FILE);
   }
   catch (std::string str) {
      std::stringstream ss;
      ss << "Errors in Configuration: " << endl;
      ss << str;
      throw ss.str(); //never throw a std type
   }
   if (m_config->error()) {
      std::string msg("Other Errors in configuration: ");
      throw msg;
   }

   m_gui = new CUserInterface(this, m_config);
   m_gui->update_options_tab();
   m_gui->attach_mutex_published_data(m_mx_published);
   m_gui->attach_signal_request(m_pub_condition_newrequest);
   m_gui->attach_signal_newdata(m_pub_condition_dataready);
   m_gui->start_thread();

   m_publisher = new CPublisher();
   m_publisher->set_thread_name("CPublisher");
   
   m_publisher->attach(m_config);
   m_publisher->attach_mutex_published_data(m_mx_published);
   m_publisher->attach_signal_request(m_pub_condition_newrequest);
   m_publisher->attach_signal_newdata(m_pub_condition_dataready);
   m_gui->attach(m_publisher);
   m_publisher->start_thread();


//   bip::message_queue::remove("mmdaq1_srv_command");
//   // Create a pool of threads to run all of the io_services.
//   std::vector<boost::shared_ptr<boost::thread> > threads;
//   for (std::size_t i = 0; i < 1; ++i)
//   {
//      boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_io_service)));
//      threads.push_back(thread);
//   }
//   
//   m_io_service.post(boost::bind(&CMMDaq::run_ipcq_command_receiver, this));
}


CMMDaq::~CMMDaq()
{
   m_busy = false; //force execution of stop()


   cout << "CMMDaq::~CMMDaq()" << endl;
   stop();

//   m_io_service.stop();
//   bip::message_queue::remove("mmdaq1_srv_command");
   
   if (m_gui && m_publisher) {
      m_gui->detach(m_publisher);
   }

   if (m_publisher) {
      m_publisher->cancel_thread();
      m_publisher->join_thread();
      //m_publisher->detach_signal_request(m_pub_condition_newrequest); //unnecessary the 2
      m_publisher->detach_signal_newdata(m_pub_condition_dataready);
      m_publisher->detach_mutex_published_data(m_mx_published);
      m_publisher->detach(m_config);
      //m_publisher->finish();
      delete m_publisher; m_publisher = 0;
   }

   if (m_gui) {
      m_gui->cancel_thread();
      m_gui->join_thread();
      m_gui->detach_signal_request(m_pub_condition_newrequest); //unnecessary the 2
      m_gui->detach_signal_newdata(m_pub_condition_dataready);
      cout <<  "m_gui->detach_mutex_published_data()" << endl;
      m_gui->detach_mutex_published_data(m_mx_published);

   }

   finish();



   FreeClear(m_datacontainer);
   FreeClear(m_eventcontainer);

   delete m_mx_published; m_mx_published = 0;

   delete m_condition_processed_mx; m_condition_processed_mx = 0;
   delete m_condition_processed; m_condition_processed = 0;
   delete m_pub_condition_newrequest_mx; m_pub_condition_newrequest_mx = 0;
   delete m_pub_condition_newrequest; m_pub_condition_newrequest = 0;
   delete m_pub_condition_dataready_mx; m_pub_condition_dataready_mx = 0;
   delete m_pub_condition_dataready; m_pub_condition_dataready = 0;
//   gApplication->Terminate(0);
}


void CMMDaq::init()
{
   m_mx_received  = new CMutex();
   m_mx_processed = new CMutex();

   pthread_cond_init(&m_action_event_received, NULL);
   pthread_cond_init(&m_action_event_processed, NULL);

   FreeClear(m_datacontainer);
   FreeClear(m_eventcontainer);
}




int CMMDaq::run()
{
   if (m_busy) {
      return 0;
   }

   if (m_receiver || m_writer || m_decoder) {
      cerr << "CMMDaq::run(): these threads are already running: ";
      if (m_receiver) cerr << "m_receiver ";
      if (m_writer)   cerr << "m_writer ";
      if (m_decoder)  cerr << "m_decoder ";
      cerr << endl;
      return 0;
   }
   m_busy = true;

   init();


   int gui_run_type = m_gui->gui_selected_run_type();

   CEvent::event_type_type preset_event_type = CEvent::eventTypeBad;
   switch (gui_run_type) {
      case 0:
         preset_event_type = CEvent::eventTypePhysics;
         break;
      case 1:
         preset_event_type = CEvent::eventTypePedestals;
         break;
      default:
         break;
   }

   std::cout << "CMMDaq::run() : preset evt type = " << preset_event_type << std::endl;

   if (m_config->error()) {
      cerr << "ERROR: Configuration errors. Exit." << endl;
      m_busy = false;
      return EXIT_FAILURE;
   }
   m_save_data_flag = m_gui->gui_save_data_checked();
   //cout << "CLogger" << endl;
   m_logger = new CLogger(m_config);
   m_logger->set_save_data_flag(m_save_data_flag);

   //run number
   std::stringstream ssrun;
   m_config->load_log();
   m_config->get_next_run_number_from_log();
   if (m_config->run_number() < 1) {
      //cerr << "ERROR: bad run number < 10000 .\n" << endl;
      //ssrun << "bad run#";
      m_gui->controls_run_start_failed("Bad run number.");
      m_busy = false;
      return 1;
   }
   else {
      m_gui->controls_run_started(m_config->run_number());
      cout << "Current run# " << m_config->run_number() << " ";
   }
   //time_t starttime;

   time(&m_run_start_time);



   //THREADS START HERE

   //cout << "CCReceiver" << endl;
   m_receiver = new CReceiver(m_config, &m_datacontainer, m_mx_received, m_action_event_received);
   int err = m_receiver->init();
   if (err != 0) {
      std::stringstream ss;
      ss << "Failed to start. " << m_receiver->fail_msg();
      
      m_gui->update_statusbar(ss.str().c_str(), 0);
      m_gui->controls_run_start_failed(ss.str());
      delete m_receiver; m_receiver = 0;
      m_busy = false;
      return 1;
   }
   pause_receiver(m_gui->gui_run_paused());


   m_decoder = new CEventDecoder(m_config, &m_datacontainer, &m_eventcontainer,
                                    m_mx_received, m_mx_processed,
                                    m_action_event_received, m_action_event_processed);
   m_decoder->attach(m_gui);
   m_decoder->attach(m_receiver);
   m_decoder->preset_event_type(preset_event_type);
   m_decoder->CThreadRoot::set_thread_name("m_decoder");

   m_writer = new CRootWriter(this, m_config, &m_eventcontainer, m_mx_published, m_mx_processed,
                              m_action_event_processed, m_condition_processed, m_save_data_flag);


   m_publisher->attach(m_writer);
   m_publisher->attach_mutex_processed_data(m_mx_processed);
	m_publisher->clear_statistics();


   m_writer->set_save_data_flag(m_save_data_flag);
   m_writer->CThreadRoot::set_thread_name("m_writer");

   //cout << "m_writer->set_save_data_flag(m_gui->gui_save_data_checked()) " << m_gui->gui_save_data_checked() << endl;
   /* create a new thread that will execute 'recv_main()' */

   m_writer->CThreadRoot::start_thread();		//thr_id2 = pthread_create(&thread_writer,   NULL, std::mem_fun(&CRootWriter::main), (void*)NULL);
   m_decoder->CThread::start_thread();
   m_receiver->start_thread();					//thr_id1 = pthread_create(&thread_listener, NULL, recv_main, (void*)NULL);



   std::stringstream ss;
   ss << "Started run " << m_writer->run_number();
   m_gui->update_statusbar(ss.str().c_str(), 0);


   //cout << "mmdaq running..." << endl;
   //getchar();
   //	/* lock the mutex, and wait on the condition variable, */
   //   /* till one of the threads finishes up and signals it. */
   //   pthread_mutex_lock(&action_mutex);
   //   pthread_cond_wait(&action_cond, &action_mutex);
   //   pthread_mutex_unlock(&action_mutex);
   m_busy = false;
   return 0;
}


int CMMDaq::stop()
{
   if (m_busy) {
      return 0;
   }
   cout <<  "CMMDaq::stop()" << endl;


   if (!m_receiver && !m_writer && !m_decoder) {
      return 0;
   }
   m_busy = true;

	m_config->comments(m_gui->gui_comment_line());
	
   int runnumber = m_writer->run_number();
   int numevt = m_writer->event_count();

   m_gui->controls_run_stopped();

   //int err = 0;
   cout <<  "m_publisher->clear()" << endl;



   if (m_publisher) {
      m_publisher->clear();
      cout <<  "m_publisher->detach() x3 ..." << endl;
      m_publisher->detach(m_writer);
      m_publisher->detach_mutex_processed_data(m_mx_processed);
   }



   if (m_receiver) {
      cout << "cancel m_receiver.. ";
      m_receiver->request_pause_run(false);
      m_receiver->cancel_thread(); //pthread_cancel(thread_listener);
      m_receiver->join_thread();   //pthread_join(thread_listener, NULL);
      m_receiver->finish();
      cout << "ok ";

   }

   if (m_decoder) {
      cout << "cancel m_decoder.. ";
      m_decoder->CThread::cancel_thread();
      m_decoder->CThread::join_thread();
      m_decoder->finish();
      cout << "ok ";

   }
   if (m_writer) {
      cout << "save data m_writer.. ";
      if (m_save_data_flag) {
         m_writer->root_write_file();
      }
   }

   //cout << "mmdaq closing.. data file .." << endl;

   if (m_logger) {
      //printf("saving log file ..");
      //m_logger->attach_writer(m_writer);

//      string comments = m_config->comments();
      string comments = m_gui->gui_comment_line();
      m_logger->write(runnumber, m_run_start_time, numevt, comments);
      //logger.finish();
      m_logger->close();

      //printf("ok\n");
      std::stringstream ss;
      ss << "Finished run " << runnumber << " with " << numevt << " events";
      m_gui->update_statusbar(ss.str().c_str(), 0);
      ss.str("");
      if (numevt) {
         ss << "Number of corrupted events (# udp frames):" << m_decoder->event_count_bad()
         << " (" << (double)m_decoder->event_count_bad() / (double)m_decoder->event_count() *100 << "%)";
         m_gui->update_statusbar(ss.str().c_str(), 3);
      }
   }



   if (m_writer) {
      cout << "cancel m_writer.. ";
      m_writer->CThreadRoot::cancel_thread(); // pthread_cancel(thread_writer);
      m_writer->CThreadRoot::join_thread(); //pthread_join(thread_writer, NULL);
      //cout << "m_writer= " << m_writer << endl;
      // m_writer->date_file_finish();
      cout << "ok ";
   }

   //cout << "Final check : writer event count = " << m_writer->event_count() << endl;

//   
   FreeClear(m_datacontainer);
   FreeClear(m_eventcontainer);
   
   cout << "delete m_decoder " << endl;
   delete m_decoder;	 m_decoder = 0;
   cout << "delete m_receiver " << endl;
   delete m_receiver; m_receiver = 0;
   cout << "delete m_writer " << endl;
   delete m_writer;   m_writer = 0;
   cout << "delete m_logger " << endl;
   delete m_logger;   m_logger = 0;

   finish();
   cout << "CMMDaq::stopped." << endl;
   m_busy = false;
   return 0;

}



void CMMDaq::finish()
{
   cout << "CMMDaq::finish()" << endl;
	
   FreeClear(m_datacontainer);
   FreeClear(m_eventcontainer);
	
   delete m_mx_received; m_mx_received = 0;
   delete m_mx_processed; m_mx_processed = 0;
	
	
   int rc = pthread_cond_destroy(&m_action_event_received);
   if (rc) { /* some thread is still waiting on this condition variable */
      /* handle this case here... */
      perror("CMMDaq::finish() pthread_cond_destroy(m_action_event_received)");
   }
	
   rc = pthread_cond_destroy(&m_action_event_processed);
   if (rc) { /* some thread is still waiting on this condition variable */
      /* handle this case here... */
      perror("CMMDaq::finish() pthread_cond_destroy(m_action_event_processed)");
   }
}



void CMMDaq::pause_receiver(bool val)
{
   if (m_receiver)
      m_receiver->request_pause_run(val);
}


void CMMDaq::update_internal_queue_sizes()
{

   if (m_receiver) m_internal_queue_sizes[0] = m_receiver->internal_queue_size();
   if (m_decoder)	m_internal_queue_sizes[1] = m_decoder->internal_queue_size();
   if (m_writer) {
      lock_mutex();
      if (m_writer)  m_internal_queue_sizes[2] = m_writer->internal_queue_decoded_size();
      if (m_writer)	m_internal_queue_sizes[3] = m_writer->internal_queue_size();
      unlock_mutex();
   }

   if (m_decoder) m_current_rate = m_decoder->current_rate();
}


///// called recursively 
//int CMMDaq::run_ipcq_command_receiver()
//{
//   bool error = 0;
//   try {
//      //ipc message queue commands max msg size 100 bytes
//      
////      std::cout << "CCommandReceiver::run_ipc_queue_receiver()" << std::endl;
//      boost::interprocess::message_queue msg_queue(bip::open_or_create, 
//                                                   "mmdaq1_srv_command" ,
//                                                   10 , 
//                                                   sizeof(ipc_command_type));
//      
//      unsigned int priority = 0;
////      bip::message_queue::size_type recvd_size;
//      size_t recvd_size;
//      ipc_command_type cmd = msq_ignore;
//      
//      boost::system_time timeout = boost::get_system_time() + boost::posix_time::seconds(1);
//      bool newmsg = msg_queue.timed_receive(&cmd, sizeof(cmd), recvd_size, priority, timeout);
//      if (newmsg) {
//         handle_command(cmd);
//      }
//      
//   } catch (bip::interprocess_exception& e) {
//      std::cout << "CCommandReceiver::run_ipc_queue_receiver() error : " << e.what() << std::endl;
//      error = true;
//   }
//   
//   if (!error /*&& !m_stopping*/) {
//      run_ipcq_command_receiver();
//   }
//   
//   return 0;
//   
//}



//msq_ignore,
//msq_heartbeat,
//msq_stop,
//msq_start,
//msq_reset,
//msq_configure

//void CMMDaq::handle_command(CMMDaq::ipc_command_type& commmand)
//{
//   std::cout << "handle command" << commmand << std::endl;
//   
//   if (commmand == msq_ignore) {
//      std::cout << "message invalid / not implemented" << std::endl;
//      
//   }
//   else if (commmand == msq_stop) {
//      stop();
//   }
//   else if (commmand == msq_start) {
//      run();
//   }
//   else if (commmand == msq_reset) {
//      std::cout << "message invalid / not implemented" << std::endl;
//
//   }
//   else if (commmand == msq_configure) {
//      std::cout << "message invalid / not implemented" << std::endl;
//
//   }
//   else if (commmand == msq_status) {
//      std::cout << "message invalid / not implemented" << std::endl;
//      //open queue "mmdaq3_srv_response" and answer status, #events, log line, writing?
//      //timeout
//      
//   } 
//   else {
//      std::cout << "message invalid / not implemented" << std::endl;
//   }
//   return;
//   
//}

