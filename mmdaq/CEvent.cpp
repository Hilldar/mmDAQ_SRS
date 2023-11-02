/*
 *  CEvent.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/9/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CEvent.h"

CEvent::CEvent(int number) 
:m_event_type(eventTypeBad), m_event_time(), m_event_number(number), m_event_bad(true), m_event_empty(true)
{}

CEvent::CEvent(event_type_type type) 
:m_event_type(type), m_event_time(), m_event_number(0), m_event_bad(true), m_event_empty(true)
{}

CEvent::CEvent(event_type_type type, const struct timeval* const tval) 
:m_event_type(type), m_event_time(*tval), m_event_number(0), m_event_bad(true), m_event_empty(true)
{ }

void CEvent::clear()
{
	m_event_type = eventTypeBad;
	m_event_time.tv_sec = 0;	
	m_event_time.tv_usec = 0;
	m_event_number = 0;
	m_event_bad = true;
}


//CEvent& CEvent::operator=(const CEvent& rhs)
//{
//	if (this == &rhs) {
//		return *this;
//	}
//	m_event_type = rhs.m_event_type;
//	m_event_time = rhs.m_event_time;
//	m_event_number = rhs.m_event_number;
//	return *this;
//}

