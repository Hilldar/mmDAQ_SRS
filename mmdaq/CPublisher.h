/*
 *  CPublisher.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/21/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */
#ifndef CPublisher_h
#define CPublisher_h

#include "CThread.h"
#include "CMMEvent.h"

#include <cassert>
#include <stdint.h>
#include <map>
#include <vector>

class CEvent;
class CMMEvent;
class CConfiguration;
class CRootWriter;
class TCondition;
class CMutex;

class TH1;

#ifndef PUBLISHER_REQUEST_IDs
#define PUBLISHER_REQUEST_IDs
#define PUBLISHER_REQUEST_ID_UDP 1
#define PUBLISHER_REQUEST_ID_1D 2
#define PUBLISHER_REQUEST_ID_2D 3
#define PUBLISHER_REQUEST_ID_STRIP 4
#define PUBLISHER_REQUEST_ID_PED 5
#define PUBLISHER_REQUEST_ID_STATISTICS 6
#define PUBLISHER_REQUEST_ID_2DHITMAP 7
#define PUBLISHER_REQUEST_ID_PEDESTAL 8

#endif


class CPublisher : public CThread {
	
public:
	
	enum data_reqests_flag_type {
		requesting_none = 0x0, 
		requesting_udp  = 0x00001, 
		requesting_raw  = 0x00002, 
		requesting_1d   = 0x00004, 
		requesting_2d   = 0x00008, 
		requesting_permanent = 0x10000
	};
	enum data_published_flag_type {
		published_none = 0x0, 
		published_udp  = 0x00001, 
		published_raw  = 0x00002, 
		published_1d   = 0x00004, 
		published_2d   = 0x00008, 
	};

public:
	CPublisher();
	~CPublisher();
	
	void clear();
	void clear_statistics();
	
	void attach(CRootWriter* writer) { m_writer = writer;}
	void attach(CConfiguration* config) { m_config = config;}
	void detach(CRootWriter* writer);
	void detach(CConfiguration* config);
	
	void attach_signal_request(TCondition* cond_newrequest) { m_cond_newrequest = cond_newrequest;}
	void detach_signal_request(TCondition* cond); 
	void attach_signal_newdata(TCondition* cond) { m_cond_dataready = cond;}
	void detach_signal_newdata(TCondition* cond); 
	
	void attach_mutex_processed_data(CMutex* mut) { m_mx_processed = mut;}
	void detach_mutex_processed_data(CMutex* mut);
	void attach_mutex_published_data(CMutex* mut) { m_mx_published = mut;}
	void detach_mutex_published_data(CMutex* mut);
	const std::vector<TH1*> get_published_data(int request_id) const;
   const std::map<int, TH1*>&  get_published_data_udp(int request_id) const;
	const int event_number() const { return m_event->event_number();} 
	
private:
	CPublisher(const CPublisher&);
	CPublisher& operator=(const CPublisher&);
	int main(void*);	//main loop for thread execution
	virtual int execute_thread() { return main((void*)0); }; 
	
	void delete_histograms( std::vector<TH1*>& vec);

   void split_mbt0_hack();

	void init_statistics();
	void fill_statistics();
	void create_udp_data();
	void create_1d_data();
	void create_2d_data();
   void create_hitmap_data();
	
private:
	CConfiguration* m_config;
	CRootWriter*	m_writer;				//get current event form here
	CEvent*			m_event;					//current event - CEvent is the base class (now it is CMMEvent)
	CEvent*			m_statistics_event;	//CEvent is the base class (now it is CMMEvent)
	TCondition*		m_cond_newrequest;
	TCondition*		m_cond_dataready;
	CMutex*			m_mx_processed;
	CMutex*			m_mx_published;
	
	std::map<int, std::vector<TH1*> > m_published; //request_id, published data
   std::map<int, TH1*>  m_published_udp; // chip,published data
	
};




/*
 
 
 
 void update(const CMMEvent& event); // update internal data with this new event
 
 const std::vector< std::vector<int16_t> >		published_udp_data() ;
 const std::map< int, std::vector<int16_t> >		published_1d_data() ;
 const std::map< int, std::vector<int16_t> >		published_2d_data() ;
 
 void			request_set  ( data_reqests_flag_type what);
 void			request_unset( data_reqests_flag_type what) ;
 const bool	request_check( data_reqests_flag_type what) const ;
 
 const bool advertisement_check ( data_published_flag_type what) const ;	
 void advertisement_set  ( data_published_flag_type what);
 void advertisement_unset( data_published_flag_type what);
 
 private:
 CPublisher(CPublisher&);
 CPublisher& operator=(CPublisher&);
  
 private:	
 int			m_data_requests_flags;
 int			m_data_published_flags;
 CMMEvent		m_mmevent;
 
 std::vector<   std::vector<int16_t> >		m_published_udp_data;
 std::map< int, std::vector<int16_t> >		m_published_1d_data;		//apvidch, qmax, tbmax, 
 std::map< int, std::vector<int16_t> >		m_published_apv2d_data;	//apvidch, q_vec
 
 void publish_udp(CMMEvent* mmevent); 
 void publish_apv1d(CMMEvent* mmevent); 
 void publish_apv2d(CMMEvent* mmevent); 
 void publish_event_data(CMMEvent* mmevent); //TODO: selcetion of data (chamber/chip based on vector of CDetElements)
 
 
 */

#endif

