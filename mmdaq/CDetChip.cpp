/*
 *  CDetChip.cpp
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CDetChip.h"

#include "MBtools.h"

#include <iostream>
#include <string>
#include <map>
#include <utility>


using std::string;
using std::cout;
using std::endl;
using std::map;
using std::pair;

CDetChip::CDetChip( int i, int chipid, const std::string n)
: CDetElement(i,n), m_serial("") , m_chip_id(chipid)
{
	if(n.empty()) { 
		string str = "APV_" + NumberToString(  get_id() );
		name(str.c_str());
	}
}

CDetChip::CDetChip(const CDetChip& rhs) 
: CDetElement(rhs), m_serial(rhs.m_serial), m_chip_id(rhs.m_chip_id)
{
	
}

CDetChip::~CDetChip() 
{
	//cout << " ~CDetChip(): "<< m_name << " m_connects.size=" << m_connects.size() << endl;

}

CDetChip& CDetChip::operator=(const CDetElement& rhs)
{
	if (this!= &rhs) {
		this->CDetElement::operator=(rhs);
	}
	return *this;
}

CDetChip& CDetChip::operator=(const CDetChip& rhs)
{
	if (this!= &rhs) {
		this->CDetElement::operator=(rhs);
		m_serial = rhs.m_serial;
		m_chip_id = rhs.m_chip_id;
	}
	return *this;
}


void CDetChip::connect_fec( CDetElement* elem)
{
	if (!m_connectors_top.empty()) {
		m_connectors_top.clear();
	}
	
	CDetElement::connect(elem, CONNECT_TOP); // connect on top
	//make unique chipId (aka apvId) 
	m_id = (elem->get_id() << 4) | get_id();
}

void CDetChip::disconnect_fec()
{
	if (!m_connectors_top.empty()) {
		m_connectors_top.clear();
	}
	m_id = -1;
}

const CDetElement* const CDetChip::fec() const
{
	if (!m_connectors_top.empty()) {
		return m_connectors_top.begin()->second;
	} 
	return 0;
}


//void CDetChip::disconnect() 
//{
//	assert(isconnected());
//	map<int, CDetElement*>::iterator iter = m_connectors_bottom.begin();
//	int numid =  iter->second->id();
//	CDetElement::disconnect(numid);
//}


const bool CDetChip::isconnected() const 
{
	return m_connectors_bottom.size(); //connected to smth
}


//const int CDetChip::get_chamber_strip(int chNum)
//{
//	CDetAdapterBoard* ab = dynamic_cast<CDetAdapterBoard*> ( m_connectors_bottom[DET_CHIP_CONNECTOR_ID_ONE] );
//	return ab->channel_to_strip(chNum);
//}

const void CDetChip::print() const
{

	cout << "name: " << m_name << " chip_id:" << m_id << " : " << " fec_id=" <<fec()->get_id() << " : ";

	std::string isconnectedto = m_connectors_top.size() ? m_connectors_top.begin()->second->name():"No ";
	std::cout << "  SN: "  << m_serial << " , top C: "	<<  isconnectedto << " ";
	
	isconnectedto = m_connectors_bottom.size() ? m_connectors_bottom.begin()->second->name():"No ";
	std::cout << " , btm C: "	<<  isconnectedto << endl;
	
}
