/*
 *  CMMDaq.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/27/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CMMDaq_h
#define CMMDaq_h

//#include "TQObject.h"
//#include <RQ_OBJECT.h>

#include "CThreadRoot.h"

//#include <boost/asio.hpp>
#include <list>
#include <map>
#include <vector>
#include <utility>
#include <pthread.h>


//#include "CApvEventDecoder.h"

class CConfiguration;
class CUserInterface;
class CLogger;
class CReceiver;
class CPublisher;
class CEventDecoder;
//class CApvEventDecoder;
class CRootWriter;
//class CSubscriber;
class CEvent;
class CMMEvent;
class CUDPData;
class CMutex;


class TMutex;
class TCondition;

class CMMDaq : public CThreadRoot
{

public:
   enum data_type_t {
      dt_no_data	= 0x00, 
		dt_udp_data	= 0x01, 
		dt_raw_data = 0x02, 
		dt_1d_data	= 0x04, 
		dt_2d_data	= 0x08, 
		dt_permanent = 0x10000
   };
   
//   typedef enum {
//      msq_ignore,
//      msq_heartbeat,
//      msq_stop,
//      msq_start,
//      msq_reset,
//      msq_configure,
//      msq_status
//   } ipc_command_type;
   

public:
   CMMDaq(int argc, char * argv[]);
   ~CMMDaq();
   int run();
   int stop();

//   void request_data(int datatype);
//   bool check_published_data_ready(data_type_t datatype);
//   std::vector< std::vector<int16_t> >		get_published_udp_data();
//	std::map< int, std::vector<int16_t> >	get_published_apv1d_data();
//   std::map< int, std::vector<int16_t> >	get_published_apv2d_data();
   long internal_queue_size(size_t num) {
      return m_internal_queue_sizes[num];
   }
   void set_internal_queue_size(size_t num, long val) {
      m_internal_queue_sizes[num] = val;
   }
   void update_internal_queue_sizes();
   bool ready_to_exit;
	//void current_rate(float val) { m_current_rate = val;}
	const float current_rate() const { return m_current_rate;}
	
	void pause_receiver(bool val); 
	
private:
	CMMDaq(CMMDaq&);
	CMMDaq& operator=(const CMMDaq&);
	virtual int execute_root_thread() {
      return main((void*)0);
   };
	int main(void*) { return 0;};
//	void root_gui_thread();

//   void handle_command(ipc_command_type& commmand);

   
private:
   CConfiguration*	m_config;
   CLogger*				m_logger;
   CReceiver*			m_receiver;
//   CApvEventDecoder* m_decoder;
   CEventDecoder*    m_decoder;
   CRootWriter*			m_writer;
	CPublisher*			m_publisher;
	
   /// The io_service used to perform asynchronous operations.
//   boost::asio::io_service m_io_service;
   
   CUserInterface*	m_gui;
//	CSubscriber*		m_subscriber;

   std::list <CUDPData*>	m_datacontainer;
   std::list <CMMEvent*>	m_eventcontainer;	// used by decoder and writer CMMEvent*
   time_t						m_run_start_time;
	float m_current_rate;

	CMutex* m_mx_received;
	CMutex* m_mx_processed;
	CMutex* m_mx_published;
	
	//TODO: thing like that
	/*
	 class { std::list <CUDPData*>	m_datacontainer; CMutex* m_mx_received; }
	 */
	
   /* global condition variable for our program. assignment initializes it. */
   pthread_cond_t  m_action_event_received;
   pthread_cond_t  m_action_event_processed;
	
	TMutex*			m_condition_processed_mx;
	TCondition*		m_condition_processed;

	TMutex*			m_pub_condition_newrequest_mx;
	TCondition*		m_pub_condition_newrequest;

	TMutex*			m_pub_condition_dataready_mx;
	TCondition*		m_pub_condition_dataready;
	
//	TMutex*			m_mx_;
	
   std::vector <long>		m_internal_queue_sizes;
   bool m_save_data_flag;
	bool m_busy;
private:
	
	void init();
   void finish();
//   int run_ipcq_command_receiver();

};

#endif
