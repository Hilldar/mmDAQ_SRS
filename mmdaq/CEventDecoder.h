//
//  CEventDecoder.h
//  mmdaq
//
//  Created by Marcin Byszewski on 9/20/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//

#ifndef mmdaq_CEventDecoder_cpp
#define mmdaq_CEventDecoder_cpp

#include "CThread.h"
#include "CThreadRoot.h"
#include "CEvent.h"
#include "CMMEvent.h"
#include "CMutex.h"

class CConfiguration;
class CUserInterface;
class CReceiver;
class CUDPData;
class CMMEvent;

/**
 Abstract base class for clases translating UDP data to events
 */
class CEventDecoder : public CThread, public CThreadRoot
{
protected:
   CConfiguration* m_config;
	CUserInterface* m_gui;
	CReceiver* m_receiver;
	std::list<CUDPData*>* m_receiver_data; ///< pointer to data storage used by receiver
	std::list<CUDPData*>  m_local_frames;  ///< local data to process
   std::list<CMMEvent*>* m_mm_events; ///< pointer to data storage used by writer
   std::list<CEvent*>    m_events; ///< local copy of built events
   
   //threads
   CMutex* m_recv_data_mutex;
	CMutex* m_processed_data_mutex;
	pthread_cond_t* m_action_event_received;
	pthread_cond_t* m_action_event_processed;
   
   
   //event counting
   int m_event_counter;
   int m_runnumber;
	int m_current_event_type;
   int m_bad_event_counter_numframes;     ///< bad event counter
   int m_bad_event_counter_empty;         ///< bad event counter
	int m_bad_event_counter_timebin_bad;   ///< bad event counter
   float m_event_rate;
   size_t m_internal_queue_size; // was public
   
   bool   m_suppressing_zeros;
	size_t m_number_of_chips;
   
   
   //TODO: make a class from this
   int m_timeOfLastProgressPrint ;  // progress print
	int m_outCtrProgressPrint ;      // progress print
   int m_prev_event_counter_at_timer;
   size_t m_event_frame_count;      // progress print
   time_t m_time_previous_event;    // progress print
   
   //equipment type
   uint32_t m_equipment_type;
   
private:
   
   void print_progress(int val);
//   void erase_local_frames(std::list<CUDPData*> :: iterator first, 
//                           std::list < CUDPData*> :: iterator last);
//	void erase_local_frames(std::list<CUDPData*> :: iterator fafa_iter);
	void erase_first_end_of_event();
   
   void convert_frames_to_events();
   bool check_frame_count( std::list<CUDPData*>& event_frames);
   void build_event(const CUDPData* udpdata);
   int decode_frame(const CUDPData* udpdata, std::vector<uint32_t>& data32Vector,unsigned& bad_event_error_code);
   int find_fec_by_ipaddress(const std::string& source_ip_address);
   
   
   inline uint16_t endian_swap16(uint16_t x)
	{
		return (x>>8) | (x<<8);
	}
	
	inline uint32_t endian_swap32(uint32_t x)
	{
		return (x>>24) | 
		((x<<8) & 0x00FF0000) |
		((x>>8) & 0x0000FF00) |
		(x<<24);
	}
   
   
public:
   CEventDecoder(CConfiguration* config, 
                 std::list <CUDPData*>* datacontainer,
                 std::list <CMMEvent*>* eventcontainer,
                 CMutex* recv_data_mutex, 
                 CMutex* processed_data_mutex,
                 //CMutex* publisher_mutex,
                 pthread_cond_t& action_event_received, 
                 pthread_cond_t& action_event_processed);
   virtual ~CEventDecoder();
   
   
   //derived from CThread
   virtual int execute_thread() {  return main((void*)0); };
	virtual int execute_root_thread() {
      return main((void*)0);
   };
   int main(void*);
   
  
//   virtual void convert_frame() = 0; ///< converting function 
   
   void attach(CUserInterface* gui) { m_gui = gui;}
   void attach(CReceiver* receiver) { m_receiver = receiver;}
   void preset_event_type(CEvent::event_type_type type) { m_current_event_type = type;}
   void finish() { m_recv_data_mutex->unlock(); }
   const int event_count_bad() const { return m_bad_event_counter_numframes;}
   const int event_count() const { return m_event_counter;}
   const float current_rate() const { return m_event_rate;} 
   size_t internal_queue_size() const { return m_internal_queue_size;}
   
   
};

#endif
