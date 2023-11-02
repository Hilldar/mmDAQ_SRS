/*
 *  CDetFec.cpp
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CDetFec.h"
#include "CDetChip.h"
//#include "CDetAdapterBoard.h"

#include "MBtools.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <map>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;
using std::map;
using std::pair;


CDetFec::CDetFec()
: CDetElement(), m_ipaddress_string(""), m_ipaddress(0)//, m_chips(0)
{}

CDetFec::CDetFec(int i, const std::string n)
: CDetElement(i,n), m_ipaddress_string(""), m_ipaddress(0)//, m_chips(0)
{
	if(n.empty()) { 
		string str = "FEC_" + NumberToString(get_id());
		name(str.c_str());
	}
}

CDetFec& CDetFec::operator=(const CDetElement& rhs)
{
	if (this != &rhs) {
		CDetElement::operator=(rhs);
	}
	return *this;
}

CDetFec& CDetFec::operator=(const CDetFec& rhs)
{
	if (this != &rhs) {
		CDetElement::operator=(rhs);
		// assign fec members
		m_ipaddress_string = rhs.m_ipaddress_string;
		m_ipaddress = rhs.m_ipaddress;
	}
	return *this;
}


CDetFec::~CDetFec()
{
//	cout << " ~CDetFec():" << m_name << " m_connects.size=" << m_connects.size() << endl;

}

void CDetFec::connect(CDetElement* c) 
{ 
	CDetElement::connect(c, CONNECT_BOTTOM);
//	assert(c != NULL);
//	assert(c->id() >= 0);
//	int connId = c->id()+1;
//	m_connectors_bottom.insert(pair<int,CDetChip*>(connId, new CDetChip(*c) )); //in fec only bottom, and connectorId = apvId+1
}


const void CDetFec::print() const
{
	//CDetElement::print();
	cout << "FEC name:" << m_name << " id:" << m_id << " IP:"<< m_ipaddress_string <<" ,number of chips:" << number_of_chips()<< endl;
	for (map <int, CDetElement*> ::const_iterator it = m_connectors_top.begin(); it!=m_connectors_top.end(); ++it) {
		cout << "top C"<<it->first << " " << it->second->name() << " " << it->second->get_id() << endl;
	}
	for (map <int, CDetElement*> ::const_iterator it = m_connectors_bottom.begin(); it!=m_connectors_bottom.end(); ++it) {
		cout << "btm C"<<it->first << " " << it->second->name() << " " << it->second->get_id() << endl;
	}
}


