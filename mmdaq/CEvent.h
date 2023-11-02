/*
 *  CEvent.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/9/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CEvent_h
#define CEvent_h
//#include <TROOT.h>

#include <sys/time.h>

class TH1;
class TH2;
class CDetAbstractReadout;


class CEvent {
	
public:
	enum event_type_type {
      eventTypeBad = 0,
      eventtypeStartOfRun = 1,
      eventtypeEndOfRun = 2,
      eventTypePhysics = 7,
      eventTypeCalibration = 8,
      eventTypePedestals = 14
   };
	
	
public:
	CEvent(int number = 0);
	CEvent(event_type_type type);
	CEvent(event_type_type type, const struct timeval* const tval);
//	CEvent(CEvent &rhs);
	virtual ~CEvent() { };
   
   virtual CEvent* clone() const  = 0; //{ return new CEvent(*this); }
   
   virtual bool is_empty()					const { return m_event_empty; }
   virtual void set_empty(bool val) { m_event_empty = val; }

	virtual void       set_bad(bool val)				{ m_event_bad = val; }	
	virtual const bool is_bad()							const { return m_event_bad; }
	const event_type_type			event_type()		const { return  m_event_type;}
	const struct timeval* const	event_time()		const { return &m_event_time;}
	const time_t						event_time_sec()	const { return m_event_time.tv_sec;}; 
	const suseconds_t					event_time_usec() const { return m_event_time.tv_usec;};
	const int							event_number()		const { return m_event_number;}
	
	void event_type(event_type_type type)	{ m_event_type = type;}
	void event_number(int number)			{ m_event_number = number;}
	virtual void print() const = 0;
	
   virtual void fill_th1_qt(TH1* h1, TH2* h2, const CDetAbstractReadout* readout) const {};
   virtual void fill_th2_qt(TH2* hist,  const CDetAbstractReadout* readout) const {};


   
protected:
	event_type_type	m_event_type;
	struct timeval		m_event_time;
	int					m_event_number;
	bool					m_event_bad;
   bool					m_event_empty;


protected:
		void clear();
	//ClassDef(CEvent, 0)

};

#endif

