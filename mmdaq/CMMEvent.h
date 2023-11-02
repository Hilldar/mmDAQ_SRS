/*
 *  CMMEvent.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 2/14/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CMMEvent_h
#define CMMEvent_h

#include "CApvEvent.h"

#include <ctime>
#include <list>
#include <fstream>
#include <stdint.h>
#include <sys/time.h>

class CEvent;
class CApvEvent;
class TFile;


class CMMEvent : public CEvent {
	
public:
	CMMEvent();
   CMMEvent( int runnumber, int eventNo);
	CMMEvent( int runnumber, int event_number, const std::list <CEvent*>& evt_list);
	CMMEvent(const CMMEvent& rhs);
	virtual ~CMMEvent();
	CMMEvent& operator=(const CMMEvent& rhs);
	
   virtual CMMEvent* clone() const  { return new CMMEvent(*this); }

	
	CEvent* add_event(const CEvent::event_type_type evtype, int apvId);
	const size_t number_of_apv_events() const { return m_events.size();} 
	const size_t pedestal_size() const;
	const std::list< CEvent* >& apv_events() const { return m_events;}; //CApvEvent*
	const CApvEvent* const	get_apv_event(int apv_id) const; //CApvEvent*
			CApvEvent*			get_apv_event(int apv_id);//CApvEvent*
	const int max_timebin_count() const { return m_max_timebin_count; };
	const int presamples_count() const;
	void clear();
	void print() const;
	//CMMEvent& deepcopy(const CMMEvent& rhs);
	void add_pedestal(const unsigned apvChId, const std::vector <double> pedvec);

   bool is_all_empty() const { return m_all_empty;}
   bool is_all_good() const { return m_all_good;}
   bool is_bad_tb_count() const { return m_bad_event_timebin_count;}
   
private:
	int m_run_number;
	//int m_event_number;
	int m_start_time_us;
	int m_max_timebin_count;
	std::list< CEvent* > m_events; //CApvEvent*
	bool m_all_empty;
   bool m_all_good;
   bool m_bad_event_timebin_count;
   
	//int m_last_number_of_timebins;
};

#endif

