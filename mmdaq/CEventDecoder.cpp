//
//  CEventDecoder.cpp
//  mmdaq
//
//  Created by Marcin Byszewski on 9/20/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//
// decode_frame() based on Kondo Gnavo's MTS Amore agent

#include "CEventDecoder.h"

#include "CReceiver.h"
#include "CUDPData.h"
#include "MBtools.h"
#include "CUserInterface.h"
#include "CDetFec.h"

#include "ProgException.h"

#include <errno.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#define MMDAQ_ERROR_DATA_NO_ERROR 0
#define MMDAQ_ERROR_DATA_FRAME_LOST 1
#define MMDAQ_ERROR_DATA_TRUNCATED 2
#define MMDAQ_ERROR_NO_SRS_MARKERS 4
#define MMDAQ_ERROR_APV_NUMBER_OUT_OF_RANGE 8
#define MMDAQ_ERROR_NUMBER_OF_TIMEBINS_OUT_OF_RANGE 16
#define MMDAQ_ERROR_FEC_IP_NOT_FOUND 32
#define MMDAQ_ERROR_FEC_IP_BAD_VALUE 64
#define MMDAQ_ERROR_DATA_BAD 128


#ifndef SRS_NEXT_EVENT
#define SRS_NEXT_EVENT 0xFAFAFAFA
#endif


//ADC raw all data
#define SRS_HEADER_HWTYPE_APV_RAW 0x00414443
//ADZ APV zero suppressed: APZ
#define SRS_HEADER_HWTYPE_APV_ZSP 0x0041505A
//#define SRS_HEADER_HWTYPE_APV_ZSP_ENDSWP 0x5a5041
//ADZ : 0x41445A


CEventDecoder::CEventDecoder(CConfiguration* config, 
                             std::list <CUDPData*>* datacontainer,
                             std::list <CMMEvent*>* eventcontainer,
                             CMutex* recv_data_mutex, 
                             CMutex* processed_data_mutex,
                             pthread_cond_t& action_event_received, 
                             pthread_cond_t& action_event_processed)
: CThread(), CThreadRoot(), 
m_config(config), m_gui(0), m_receiver(0), m_receiver_data(datacontainer),  
m_local_frames(), m_mm_events(eventcontainer), m_events(),

m_recv_data_mutex(recv_data_mutex), m_processed_data_mutex(processed_data_mutex),
m_action_event_received(&action_event_received),
m_action_event_processed(&action_event_processed),
m_event_counter(0), m_runnumber(m_config->run_number()), 
m_current_event_type(0), 
m_bad_event_counter_numframes(0), m_bad_event_counter_empty(0), m_bad_event_counter_timebin_bad(0),
m_event_rate(0.0),
m_internal_queue_size(0),
m_suppressing_zeros(m_config->suppressing_zeros()),
m_number_of_chips(m_config->chip_count()),

//TODO: make a class from this
m_timeOfLastProgressPrint(-1), m_outCtrProgressPrint(0), m_prev_event_counter_at_timer(0),
m_event_frame_count(0), m_time_previous_event(0),
m_equipment_type(0)

{
//   m_apv_events.clear();
   CThread::set_thread_name("m_decoder");
}

CEventDecoder::~CEventDecoder()
{
   FreeClear(m_local_frames);
   FreeClear(m_events);
}



int CEventDecoder::main(void*)
{
   //dont't:
	/* make sure we're in asynchronous cancelation mode so   */
   /* we can be canceled even when blocked on reading data. */
   //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
   
   struct timeval  now;            /* time when we started waiting        */
   struct timespec timeout;        /* timeout value for the wait function */
   
   /* timespec uses nano-seconds.  1 micro-second = 1000 nano-seconds. */
   m_recv_data_mutex->lock(thread_id(), thread_name() , "main(), 1");
	/* mutex is now locked - wait on the condition variable.             */
	/* During the execution of pthread_cond_wait, the mutex is unlocked. */
	
   while (1) {
      gettimeofday(&now, NULL);
      // prepare timeout value. Note that we need an absolute time.
      timeout.tv_sec = now.tv_sec + 1 ;
      timeout.tv_nsec = now.tv_usec * 1000; /* timeval uses micro-seconds. */
      
      int rc = 0;
      if (!m_receiver->event_ready_check()) {
         /* remember that pthread_cond_timedwait() unlocks the mutex on entrance */
         rc = m_recv_data_mutex->timedwait(m_action_event_received, &timeout, thread_name());
      }
      
      /** RECEIVER DATA IS LOCKED **/
      //      int ii = 0;
      switch (rc) {
         case 0:  
            /* we were awakened due to the cond. variable being signaled */
            // the mutex was now locked again by pthread_cond_timedwait.
            // we were signaled that receiver received FAFAFAFA
            
            m_local_frames.splice(m_local_frames.end(), *m_receiver_data);
            //do not delete the CUDPData* here from m_receiver_data , they are passed as pointers to local storage
            // they are deleted from local_frames when constructing apvevents
            
            m_receiver->event_ready_unset();
            m_internal_queue_size = m_local_frames.size();
            
				/* UNLOCK RECEIVER DATA UNLOCKED */
            m_recv_data_mutex->unlock();
            /* temporarly unlock m_receiver_data for receiver access while processing local data**/
            
            
            convert_frames_to_events();
//            convert_frames_to_apv_events();
				
            
            
            //signal to Writer that we have processed an event
            if (!m_mm_events->empty()) {
               CThread::signal_condition(m_action_event_processed);
            }
            
            print_progress(0);
            CThread::test_cancel_thread();
            
            /** LOCK BACK RECEIVER DATA **/
            m_recv_data_mutex->lock(thread_id(), thread_name(), "main(), 2");
            break;
            
         default:        /* some error occurred (e.g. we got a Unix signal) */
            print_progress(0);
            if (errno == ETIMEDOUT) { /* time out */
               //print_progress(0);
               m_internal_queue_size = m_local_frames.size();
            }
            break; 
      }//switch
   }
   //not reached
   return 0;
}


void CEventDecoder::print_progress(int val)
{
   
   // if (m_err) return;
   char outFlg[] = { '|', '/', '-', '\\', 0 };
   int now = (int)time(NULL);
   if (m_timeOfLastProgressPrint == -1) m_timeOfLastProgressPrint = now;
   if (m_timeOfLastProgressPrint != -1) {
      if (now - m_timeOfLastProgressPrint >= 1) {
         m_timeOfLastProgressPrint = now;
         
         time_t m_time_current = 0;
         time(&m_time_current);
         time_t elapsed_time = m_time_current - m_time_previous_event;
         
         if (elapsed_time) {
            float rate = ((float)(m_event_counter - m_prev_event_counter_at_timer) / (float)(elapsed_time));
            m_event_rate = (m_event_rate + rate) / 2;
         }
         
         m_time_previous_event = m_time_current;
         m_prev_event_counter_at_timer = m_event_counter;
         if (m_gui) {
            std::stringstream ss;
            ss << "Events Received "  << m_receiver->get_received_events_count() 
            << " Processed " << m_event_counter << ", Rate " << std::setw(5)<< std::fixed << std::setprecision(2) << m_event_rate;
            
            std::stringstream ss2;
            ss2 << "Queue in " << m_internal_queue_size;
            std::stringstream ss3;
            ss3 << "bad events:"
            << " #frames=" <<	m_bad_event_counter_numframes
            << " #tb=" <<	m_bad_event_counter_timebin_bad  //TODO: this is 0 in this class : virtual get_progress_string()
            << " empty=" << m_bad_event_counter_empty
            << " skipped=" << m_receiver->get_ignored_events_count();;
            
            m_gui->update_statusbar(ss.str().c_str(), 0); // print_progress(queuesize);
            m_gui->update_statusbar(ss2.str().c_str(), 2);
            m_gui->update_statusbar(ss3.str().c_str(), 3);
            
         }
         else {
            printf("\r%c events=%5d, rate= %5.2f, queue_in=%5d, apv_count=%5lu, bad_events=%5d         ",
                   outFlg[ m_outCtrProgressPrint++ ], m_event_counter, m_event_rate, val, m_event_frame_count, m_bad_event_counter_numframes);
         }
         if (outFlg[ m_outCtrProgressPrint ] == 0) m_outCtrProgressPrint = 0;
         fflush(stdout);
      }
   }
}


void CEventDecoder::convert_frames_to_events()
{
   FreeClear(m_events);
   if (!m_local_frames.size()) {
      return ;
   }
   bool found_fafa = false;
   
//   std::cout << " CEventDecoder::convert_frames_to_events() received  event = " << m_receiver->get_received_events_count() << std::endl;
//   std::cout << " CEventDecoder::convert_frames_to_events() processed event = " << m_event_counter << "  before while "<< std::endl;

   
   while(1) {
      CThread::test_cancel_thread();
      found_fafa = false;
      
      CUDPData* fafa_udp = 0;
      std::list<CUDPData*> ::iterator fafa_iter = std::find_if(m_local_frames.begin(), 
                                                          m_local_frames.end(),
                                                          std::mem_fun(&CUDPData::is_fafa));
      

      if (fafa_iter == m_local_frames.end() ) {
         if (!m_local_frames.empty()) {
            std::cerr << "Warning: event " << m_event_counter << " received "
            << m_local_frames.size() << " frames but no fafa frame. Ignore if no error follows." << std::endl;
         }
         // that can happen sometimes 
         break; //m_local_frames do not have the fafaiter ignore them (should we delete them?)
      }
      fafa_udp = *fafa_iter;
      found_fafa = true;
      
      ++m_event_counter;
//      std::cout << " CEventDecoder::convert_frames_to_events() processed event = " << m_event_counter <<  " within" << std::endl;
      
//      std::cout << m_event_counter << ": local frames size=" << m_local_frames.size() << std::endl;
      std::list<CUDPData*> event_frames;
      event_frames.splice(event_frames.end(), m_local_frames, m_local_frames.begin(), fafa_iter);
//      std::cout << m_event_counter << ": event_frames size=" << event_frames.size() << std::endl;
      
      delete fafa_udp; fafa_udp = 0;                  // fafa_iter is not moved to event_frames, delete and erase
      m_local_frames.erase(m_local_frames.begin());   // remove frame pointed to previously by fafa_iter
//      std::cout << m_event_counter << ": local frames size=" << m_local_frames.size() << std::endl;
//      std::cout << m_event_counter << ": event_frames size=" << event_frames.size() << std::endl;

      
      bool bad_apv_count = check_frame_count(event_frames);
      
      
      if (!bad_apv_count) {
         //build events and push into m_apv_events
         std::for_each(event_frames.begin(), event_frames.end(), 
                       std::bind1st(std::mem_fun(&CEventDecoder::build_event), this)  );
//         try {
//         pthread_t tid = thread_id();
         std::string tnm =  thread_name();
//         std::cout <<"thread name: " << tnm <<" test lock m_processed_data_mute: " << m_processed_data_mutex->check_lock() << std::endl;
//         m_processed_data_mutex->unlock();
         m_processed_data_mutex->lock(thread_id(), thread_name(), "convert_frames(), 1"); //terminates
//         } catch (CProgException& e) {
//            std::cerr << "CEventDecoder::convert_frames_to_events() lock m_processed_data_mutex: " << e << std::endl;
//         }
         
         CMMEvent* mmevent = new CMMEvent(m_runnumber,  m_event_counter, m_events); //copies pointers
         m_events.clear();
         
         m_mm_events->push_back(mmevent);
         m_processed_data_mutex->unlock();
         
         if (mmevent->is_bad_tb_count() ) {
            ++m_bad_event_counter_timebin_bad;
         }
         if (mmevent->is_all_empty()) {
            ++m_bad_event_counter_empty;
         }
      } //if (!bad_apv_count)
      else {
         //TODO: lock?
         
         m_processed_data_mutex->lock(thread_id(), thread_name(), "convert_frames(), 1"); //terminates
         m_mm_events->push_back(new CMMEvent(m_runnumber,  m_event_counter));
         m_processed_data_mutex->unlock();

         // ignore and clear the avp events due to bad apv count 
			// cout << "CApvEventDecoder::convert_frames_to_apv_events(): bad_apv_count" <<endl;
      }
      FreeClear(event_frames);
      m_events.clear(); //all apv events* are now in mmevent;
   }//while

}


//void CEventDecoder::erase_local_frames(std::list<CUDPData*>::iterator first, std::list<CUDPData*>::iterator last)
//{
//   //remove frames from vector
//   for (std::list < CUDPData*> ::iterator it = first; it != last; ++it) {
//      //remove data - delete allocated(in CReceiver) char* until fafa_iter
//      delete(*it);
//      (*it) = 0;
//   }
//   m_local_frames.erase(first, last); //?
//}
//
//void CEventDecoder::erase_local_frames(std::list < CUDPData*> :: iterator fafa_iter)
//{
//   delete *fafa_iter;
//   *fafa_iter = 0;
//   m_local_frames.erase(fafa_iter);
//}


void CEventDecoder::erase_first_end_of_event()
{
   std::list < CUDPData*> :: iterator first = m_local_frames.begin();
   if ((*first)->get_size() == 1) {
      if ((*first)->first_word32()  == SRS_NEXT_EVENT) {
         delete *first;
         m_local_frames.erase(first);
      }
   }
}


bool CEventDecoder::check_frame_count(std::list<CUDPData*>& event_frames)
{
   size_t evt_count = event_frames.size();
   m_event_frame_count = evt_count;
   if (m_number_of_chips != evt_count) {
      ++m_bad_event_counter_numframes;

      if (m_number_of_chips < evt_count) {
         std::cout << "ERROR: Dropped event " <<  m_event_counter << " -  too many frames (" <<  m_event_frame_count << ")" << std::endl;
         return true; // error
      }
      std::cout << "WARNING: event " <<  m_event_counter << " - incomplete event - bad frame count (" <<  m_event_frame_count << ")" << std::endl;
      return false ;//true; // ignore error less frames thn chips error = false
   }
   return false;
}


void CEventDecoder::build_event(const CUDPData* udpdata)
{
   //TODO: decide what electronics this frame is coming from 
   // build accordingly, event factory
   
   unsigned error_code = 0;
   std::vector<uint32_t> data32Vector ;
   int chip_id = decode_frame(udpdata, data32Vector, error_code);
   //TODO: decode_frame should look for and return HW type
   
   CEvent::event_type_type evtype = static_cast<CEvent::event_type_type>(m_current_event_type);
   CEvent* event =0;
   
   if (error_code || chip_id < 0) {
      //build an empty bad event 
      event = new CApvEvent(evtype, chip_id, m_equipment_type);
   }
   else {
      event = new CApvEvent(m_config, udpdata, evtype, data32Vector, chip_id, m_equipment_type );
   }
   
   m_events.push_back(event);
   //   std::cout << " CEventDecoder::build_event m_current_event_type=" << m_current_event_type <<  std::endl;
   //event->event_type( static_cast<CEvent::event_type_type> ( m_current_event_type));
   //event->print();
}


int CEventDecoder::decode_frame(const CUDPData* udpdata, std::vector<uint32_t>& data32Vector, unsigned& bad_event_error_code)
{
   
   uint32_t hw_equipment_code = 0;
   
   bool event_bad = true;
   bad_event_error_code = MMDAQ_ERROR_NO_SRS_MARKERS;
   
   int fecNo = find_fec_by_ipaddress(udpdata->get_ipaddress());
   if (fecNo < 1) {
      event_bad = true;
      bad_event_error_code |= MMDAQ_ERROR_FEC_IP_BAD_VALUE;
   }

   
   int udp_frame_number_srs = 0;
   int  apv_consecutive_number = 0, apvNo = 0;
   unsigned currentAPVPacketHdr = 0;
   int previousAPVPacketSize = 0 ;
   int udpPacketNumber = 0;
   const std::vector<uint32_t>&	m_udp_data32 = udpdata->vectored_data();
   data32Vector.reserve(m_udp_data32.size()) ;
   
   //	if (m_event_bad) {
   //		return -1;
   //	}
   
   // current_offset++; //jump over the first frame counter
   
//   if (m_udp_data32.size() < 4) {
//      bad_event_error_code = MMDAQ_ERROR_DATA_BAD;
////      event_bad = true;
//      return -1;
//   }

   
   int ii = 0;
   for (std::vector<uint32_t>::const_iterator idatum32 = m_udp_data32.begin(); 
        idatum32 != m_udp_data32.end(); ++idatum32, ++ii) {
      
//      std::cout << m_event_counter << "-->  decode_frame: m_udp_data32 ii=" << ii 
//      << " => " << std::hex << *idatum32 << " > " << endian_swap32(*idatum32) << std::dec << std::endl;

//      if (( (*idatum32>>8 & 0xFFFFFF) == SRS_HEADER_HWTYPE_APV_RAW || (endian_swap32(*idatum32) & 0xFFFFFF) == SRS_HEADER_HWTYPE_APV_RAW ) 
//      || (  (*idatum32>>8 & 0xFFFFFF) == SRS_HEADER_HWTYPE_APV_ZSP || (endian_swap32(*idatum32) & 0xFFFFFF) == SRS_HEADER_HWTYPE_APV_ZSP ) ) {
//         std::cout << "---  -->  this is the HW header " <<  std::hex << *idatum32 << std::dec  <<std::endl;
//      }
      //			if(ii < 220) cout << "CAPVEvent: @" << ii << " " << std::hex << *idatum32 << std::dec << endl;
      
      if (*idatum32 == SRS_NEXT_EVENT) {
         std::cerr << "UNHANDLED 0xFAFAFAFA in decode_frame" << std::endl;
         std::cerr << "data size = " << m_udp_data32.size()  << std::endl;
         apv_consecutive_number = 0 ;
         apvNo  = 0 ;
         data32Vector.clear() ;
         //current_offset++ ;
         break ;
      }
      
      
      
      // This contains the nb of word in sample = timebin * 65 (2apv /32bits words) + header)    //
      //0xaabb____: if (((*idatum32 >> 16) & 0xffff) == 0xaabb ) AND previousAPVPacketSize = (*idatum32 & 0x0000ffff) ;
      //0x____bbaa: if ((*idatum32 & 0xffff) == 0xbbaa )    AND previousAPVPacketSize = ((*idatum32 >> 16) & 0xffff) ;
      if (((*idatum32 >> 16) & 0xffff) == 0xaabb) {
         previousAPVPacketSize = ((*idatum32) & 0xffff) ;
//         std::cout << m_event_counter << "-->  decode_frame: previousAPVPacketSize " << " = " << previousAPVPacketSize << std::endl;
         //cout << " aabb @" << ii << " " << "size=" << previousAPVPacketSize << endl;
         continue ;
      }
      
      // New sample (APV channel) data in the equipment
      // 0x414443__: if (((*idatum32 >> 8) & 0xffffff) == 0x414443 ) AND apvNo  = currentAPVPacketHdr & 0x000000ff ;
      // 0x__434441: if ((*idatum32 & 0x00ffffff) == 0x434441 )       AND apvNo  = (currentAPVPacketHdr >> 24) & 0xff ;
      if(   ((*idatum32 >> 8) & 0xffffff) == SRS_HEADER_HWTYPE_APV_RAW 
         || ((*idatum32 >> 8) & 0xffffff) == SRS_HEADER_HWTYPE_APV_ZSP ) {
         m_equipment_type = (*idatum32 >> 8) & 0xffffff;
         hw_equipment_code = (*idatum32 >> 8) & 0xffffff; 
//         std::cout << m_event_counter << "-->  decode_frame: m_udp_data32 ii="<< ii 
//         << " => HW HEADER: " << std::hex << ( (*idatum32 >> 8) & 0xffffff) << std::dec << std::endl;
         currentAPVPacketHdr = *idatum32  ;
         //===================================================================================================//
         // last word of the previous packet added for Filippo in DATE to count the eventNb x 16 UDP packets  //
         // We dont need it here, will just skip it We remove it from the vector of data                      //
         //===================================================================================================//
         if (data32Vector.size()) {
            udpPacketNumber = data32Vector.back();
            udp_frame_number_srs = udpPacketNumber;
            //cout << "data32Vector.size()=" << data32Vector.size() << " popping back: " << data32Vector.back() <<endl;
            data32Vector.pop_back() ;
         }
         //=== INITIALISE EVERYTHING
         apvNo  = (currentAPVPacketHdr) & 0xff ;
         apv_consecutive_number++;
         if (apv_consecutive_number > 16) 	{
            bad_event_error_code |= MMDAQ_ERROR_APV_NUMBER_OUT_OF_RANGE;
            break ;//TODO: error handling
         }
         event_bad = false;
         bad_event_error_code &= ~ MMDAQ_ERROR_NO_SRS_MARKERS;
         
         data32Vector.clear() ;
         continue ;
      }
      
      if (hw_equipment_code == SRS_HEADER_HWTYPE_APV_RAW) {
         data32Vector.push_back(endian_swap32(*idatum32)) ; //TODO: depends on HW equipment
      }
      else if (hw_equipment_code == SRS_HEADER_HWTYPE_APV_ZSP) {
         data32Vector.push_back(*idatum32); //TODO: depends on HW equipment
      }
      
      //TODO: this is 2nd time endian_swap() (1st:UDPdata)  - change control words only
   }
   //m_apv_id = (fecNo << 4) | apvNo;
//   std::cout << "end of decode_frame() fec#:"<< fecNo << " apv#:" << apvNo << " data32Vector<unsigned> size = "<< data32Vector.size() << std::endl;
//	decode_apv_data(&data32Vector);
   
   int apv_id = CApvEvent::make_chipId(fecNo, apvNo);
   return apv_id;
}


int CEventDecoder::find_fec_by_ipaddress(const std::string& source_ip_address)
{
   int fecNo = 0;
	if (!m_config) {
		return 0;
	}
   const std::vector<CDetFec*> fecs = m_config->defined_fecs();
   for (std::vector<CDetFec*>::const_iterator ifec = fecs.begin(); ifec != fecs.end(); ++ifec) {
      if ((*ifec)->ipaddress() == source_ip_address) {
         fecNo = (*ifec)->get_id();
         return fecNo;
      }
   }
   return fecNo;
}

