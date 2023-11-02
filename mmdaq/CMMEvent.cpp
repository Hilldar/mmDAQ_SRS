/*
 *  CMMEvent.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 2/14/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CMMEvent.h"
#include "CEvent.h"
#include "CApvEvent.h"
#include "MBtools.h"


#include <TFile.h>


#include <iostream>
#include <sys/time.h>

CMMEvent::CMMEvent()
: CEvent(), m_run_number(0), m_start_time_us(0), m_max_timebin_count(0), m_events(0,0L),
m_all_empty(true), m_all_good(false), m_bad_event_timebin_count(true)
{

}

CMMEvent::CMMEvent( int runnumber, int eventNo)
: CEvent(eventNo), m_run_number(runnumber), m_start_time_us(0), m_max_timebin_count(0), m_events(),
m_all_empty(true), m_all_good(false), m_bad_event_timebin_count(true)
{ }


CMMEvent::CMMEvent( int runnumber, int eventNo, const std::list<CEvent*>& evt_list)
: CEvent(eventNo), m_run_number(runnumber),
m_start_time_us(0), m_max_timebin_count(0), m_events(evt_list),
m_all_empty(false), m_all_good(false), m_bad_event_timebin_count(false)
{   
//   m_events.splice(m_events.begin(), evt_list);

//   std::cout << "CMMEvent constructor starts " <<std::endl;
   //TODO: test we have all chip numbers
   
   //TODO: move to builder
   //APV test time bins count
	for (std::list <CEvent*> :: const_iterator ievt = evt_list.begin(); ievt != evt_list.end(); ++ievt) {
      CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievt);
		m_max_timebin_count = (m_max_timebin_count > (apvevt)->timebin_count() ) ? m_max_timebin_count : (apvevt)->timebin_count() ;
	}
   
   
	//m_events	= *evt_list;
	m_event_time	= *( (*(evt_list.begin() ) ) ->  event_time() );
	m_event_type	=    (*(evt_list.begin() ) ) ->  event_type() ;
	m_event_bad		=   ( evt_list.end() != std::find_if(evt_list.begin(), evt_list.end(), 
                                   std::mem_fun(&CEvent::is_bad)));
//   (*(evt_list.begin() ) ) ->  is_bad();
   
   
   int ii = 0;
   int last_tb_count = 0;
   //check for bad time bins ncount 
   bool bad_timebin_count = false;
   for (std::list<CEvent*>::iterator ievt = m_events.begin() ; ievt != m_events.end(); ++ievt, ++ii) {
      CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievt);
      if (!ii) {
         last_tb_count = (apvevt)->timebin_count();
      }
      else if (last_tb_count != (apvevt)->timebin_count()) {
         //bad number of timebins
         bad_timebin_count = true;
         (*ievt)->set_bad(true);
         if (ii == 1) {
            m_events.front()->set_bad(true);
         }
      }
   }

   if (bad_timebin_count) {
      m_bad_event_timebin_count = true;
   }
   
   // check for empty or bad events
   long num_empty = std::count_if(m_events.begin(), m_events.end(), std::mem_fun(&CEvent::is_empty));
   long num_bad   = std::count_if(m_events.begin(), m_events.end(), std::mem_fun(&CEvent::is_bad) );
   
   m_all_empty = !(num_empty != m_events.size());
   m_all_good  = (num_bad == 0);

}


CMMEvent::CMMEvent(const CMMEvent& rhs)
:CEvent(rhs), m_run_number(rhs.m_run_number), //m_event_number(rhs.m_event_number), 
m_start_time_us(rhs.m_start_time_us), m_max_timebin_count(rhs.m_max_timebin_count), m_events()
{
	//deep copy apv_events
	for (std::list< CEvent* > :: const_iterator it = rhs.m_events.begin(); it != rhs.m_events.end(); ++it) {
      m_events.push_back((*it)->clone());
	}
}


CMMEvent::~CMMEvent()
{
   FreeClear(m_events);
//	while (!m_events.empty()) {
//		CEvent* evt = m_events.front();
//		m_events.erase(m_events.begin());
//		delete evt; evt = 0;
//	}	
}

CMMEvent& CMMEvent::operator=(const CMMEvent& rhs)
{
	if (this == &rhs) {
		return *this;
	}
	CEvent::operator=(rhs);
	m_run_number		= rhs.m_run_number;
	m_start_time_us	= rhs.m_start_time_us;
	m_max_timebin_count = rhs.m_max_timebin_count;
   FreeClear(m_events);
//	while (!m_events.empty()) {
//		CEvent* evt = m_events.front();
//		m_events.erase(m_events.begin());
//		delete evt; evt = 0;
//	}	
	
	//deep copy apv_events
	for (std::list< CEvent* > :: const_iterator it = rhs.m_events.begin(); it != rhs.m_events.end(); ++it) {
      m_events.push_back((*it)->clone());
	}
	return *this;
}


void CMMEvent::print() const
{
	std::cout << "CMMEvent:" << "m_run_number = " << m_run_number 
	<< "m_event_number=" << m_event_number 
	<< "m_start_time_us=" << m_start_time_us << std::endl;

   std::for_each(m_events.begin(), m_events.end(), std::mem_fun(&CEvent::print));
//	for (std::list< CEvent* > :: const_iterator it = m_events.begin(); it != m_events.end(); ++it) {
//			(*it)->print();
//	}
	

}

void CMMEvent::clear()
{
   FreeClear(m_events);
   
//	while (!m_events.empty()) {
//		CEvent* evt = m_events.front();
//		m_events.erase(m_events.begin());
//		delete evt; evt = 0;
//	}
	
	CEvent::clear();
	m_run_number = 0;
	m_start_time_us = 0;
	m_max_timebin_count = 0;
  
	m_all_empty = true;
   m_all_good = false;
   m_bad_event_timebin_count = true;   
}

//TODO: change to CEvent or 'SRSEvent'
const CApvEvent* const CMMEvent::get_apv_event(int apv_id) const
{
	for (std::list< CEvent* > :: const_iterator ievent = m_events.begin(); ievent != m_events.end(); ++ievent) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievent);
		if (apvevt) {
			if (apvevt->apv_id() == apv_id) {
				return apvevt;
			}
		}
	}
	return 0;
}

//TODO: change to CEvent
CApvEvent* CMMEvent::get_apv_event(int apv_id)
{
	for (std::list< CEvent* > :: iterator ievent = m_events.begin(); ievent != m_events.end(); ++ievent) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievent);
		if (apvevt) {
			if (apvevt->apv_id() == apv_id) {
				return apvevt;
			}
		}
	}
	return 0;
}



void CMMEvent::add_pedestal(const unsigned apvChId, const std::vector <double> pedvec)
{
	int apvId = CApvEvent::chipId_from_chId(apvChId); 
	//int apvId = apvChId >> 8;
	//	int chNum = apvChId & 0xFF;
	CApvEvent* apvevt = get_apv_event(apvId);
	if (apvevt) {
		apvevt->add_pedestal(apvChId, pedvec);
		//std::cout << "CMMEvent::add_pedestal(): ---" << apvId << ", " << apvChId << ", " << pedvec[0] << ", " << pedvec[1] << std::endl;
	}
	else {
		CEvent* evt = add_event(CEvent::eventTypePedestals, apvId);
		evt->set_bad(false);
		CApvEvent* apevt = dynamic_cast<CApvEvent*> (evt);
//		apevt->apv_id(apvId);
		apevt->add_pedestal(apvChId, pedvec);
		//std::cout << "CMMEvent::add_pedestal(): +++" << apvId << ", " << apvChId << ", " << pedvec[0] << ", " << pedvec[1] << std::endl;
	}
}


CEvent* CMMEvent::add_event(const CEvent::event_type_type evtype, int apvId)
{
	CApvEvent* evt = new CApvEvent(evtype, apvId, 0);
	m_events.push_back(evt);
	return evt;
}

const size_t CMMEvent::pedestal_size() const 
{
	size_t number = 0;
	for (std::list< CEvent* > ::const_iterator it = m_events.begin(); it != m_events.end(); ++it) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*> (*it);
		number += apvevt->pedestal_size();
	}	
	return number;
}

const int CMMEvent::presamples_count() const 
{
	int number = 0;
	if (m_events.empty()) {
		return 0;
	}
	CApvEvent* apvevt = 0;
	if ( (apvevt = dynamic_cast<CApvEvent*>(*(m_events.begin() )) )) {
		number = apvevt->presamples_count();
	}
	//check for constant value of presamples in different apv events, to be disabled
	for (std::list< CEvent* > ::const_iterator it = m_events.begin(); it != m_events.end(); ++it) {
		apvevt = dynamic_cast<CApvEvent*> (*it);
		if (apvevt && number != apvevt->presamples_count() ) {
			//std::cerr << "CMMEvent::presamples_count(): presamples not constant between apv"  << std::endl; 
			return 0;
		}
	}	
	return number;
}


