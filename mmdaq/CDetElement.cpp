/*
 *  CDetElement.cpp
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CDetElement.h"
//#include	"CDetConnector.h"

#include <cassert>
#include <iostream>
#include <utility>

using std::map;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;

//int DetElementIDList::nID = 0;

CDetElement::CDetElement()
: IDs(), m_id(-1), m_name(""), m_parent(0), m_connectors_bottom(), m_connectors_top()
// m_connects(0)
{

}


CDetElement::CDetElement(int i, const std::string n)
: IDs(), m_id(i), m_name(n.empty()?"":n), m_parent(0), m_connectors_bottom(), m_connectors_top()
//, m_connects(0)
{
}


CDetElement::CDetElement(const CDetElement &e)
: IDs(), m_id(e.m_id), m_name(e.m_name), m_parent(0), 
m_connectors_bottom(e.m_connectors_bottom), m_connectors_top(e.m_connectors_top)
{
	//do shallow copy of connected elements

	
//	for (map<int, CDetElement*>::const_iterator ic = e.m_connectors_bottom.begin(); ic!= e.m_connectors_bottom.end(); ++ic) {
//		add_connector( ic->second); 
//		//m_connectors_bottom[ic->first] = new CDetElement( *(ic->second)) ;
//	}
//	for (map<int, CDetElement*>::const_iterator ic = e.m_connectors_top.begin(); ic!= e.m_connectors_top.end(); ++ic) {
//		add_connector( ic->second); 
//		//m_connectors_top[ic->first] = new CDetElement( *(ic->second)) ;
//	}
}


CDetElement::~CDetElement() 
{
	//not the owner
	
//	//cout << "  ~CDetElement(): "<< m_name << " m_connects.size=" << m_connects.size() << endl;
//	for (map<int, CDetElement*>::iterator ic = m_connectors_bottom.begin(); ic!= m_connectors_bottom.end(); ++ic) {
//		delete ic->second ;
//		ic->second=NULL;
//	}
//	for (map<int, CDetElement*>::iterator ic = m_connectors_top.begin(); ic!= m_connectors_top.end(); ++ic) {
//		delete ic->second ;
//		ic->second=NULL;
//	}
	
}


CDetElement& CDetElement::operator=(const CDetElement& e)
{

	if (this!= &e) {		
		m_id = e.m_id;
		m_name = e.m_name; 
		m_connectors_top = e.m_connectors_top;
		m_connectors_bottom = e.m_connectors_bottom;
		
		//do shallow copy of connected elements

		
		//remove existing connnectors
//		for (map<int, CDetElement*>::iterator ic = m_connectors_bottom.begin(); ic!= m_connectors_bottom.end(); ++ic) {
//			delete ic->second ;
//			ic->second=NULL;
//		}
//		for (map<int, CDetElement*>::iterator ic = m_connectors_top.begin(); ic!= m_connectors_top.end(); ++ic) {
//			delete ic->second ;
//			ic->second=NULL;
//		}
//		m_connectors_bottom.clear();
//		m_connectors_top.clear();
//		
//		//make copy of connectors from e
//		
//		for (map<int, CDetElement*>::const_iterator ic = e.m_connectors_bottom.begin(); ic!= e.m_connectors_bottom.end(); ++ic) {
//			add_connector( ic->second); 
//			//m_connectors_bottom[ic->first] = new CDetElement( *(ic->second)) ;
//		}
//		for (map<int, CDetElement*>::const_iterator ic = e.m_connectors_top.begin(); ic!= e.m_connectors_top.end(); ++ic) {
//			add_connector( ic->second); 
//			//m_connectors_top[ic->first] = new CDetElement( *(ic->second)) ;
//		}
		
	}
	return *this;
}


//void CDetElement::add_connector( CDetElement* connector) 
//{
//	CDetConnector* c = dynamic_cast<CDetConnector*> (connector);
//	if (!c) {
//		cerr << "Error: CDetElement::add_connector(): not a connector"<< endl;
//		return;
//	}
//	int connid = c->id();
//	direction_t face = c->facing();
//	if(face == CDetConnector::facing_up) 
//		m_connectors_top.insert(pair<int, CDetElement*> (connid, new CDetConnector(*c) )); 
//	else 
//		m_connectors_bottom.insert(pair<int, CDetElement*> (connid, new CDetConnector(*c) ) );
//}
//
//
//void CDetElement::del_connector(int connid, direction_t facing) 
//{ 
//	if(facing == det_face_top) 
//		m_connectors_top.erase(connid); 
//	else 
//		m_connectors_bottom.erase(connid);
//	
//}


CDetElement* CDetElement::find(int element_id) 
{
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_top.begin(); ielem!=m_connectors_top.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			return ielem->second;
		}
	}
	
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_bottom.begin(); ielem!=m_connectors_bottom.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			return ielem->second;
		}
	}
	return 0;
}

CDetElement* CDetElement::find(std::string element_name)
{
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_top.begin(); ielem!=m_connectors_top.end(); ++ielem) {
		if(ielem->second->name() == element_name) {
			return ielem->second;
		}
	}
	
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_bottom.begin(); ielem!=m_connectors_bottom.end(); ++ielem) {
		if(ielem->second->name() == element_name) {
			return ielem->second;
		}
	}
	return 0;
	
}

int CDetElement::find_connector(int element_id) 
{
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_top.begin(); ielem!=m_connectors_top.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			return ielem->first;
		}
	}
	
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_bottom.begin(); ielem!=m_connectors_bottom.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			return ielem->first;
		}
	}
	return -1;
}


//TODO: add: el->parent(this);
int CDetElement::connect( CDetElement* el, bool top_face) 
{ 
	int connId = IDs.GetUniqueID(); //will always succeed - unique number
	assert(el != NULL);
	assert(el->get_id() >= 0);
	pair< map<int, CDetElement*>::iterator, bool> rc;
	if (top_face) {
		rc = m_connectors_top.insert(pair<int, CDetElement*>(connId,el));
	}
	else {
		rc = m_connectors_bottom.insert(pair<int, CDetElement*>(connId,el));
	}
	el->parent(this);
	return rc.second - 1; // 0 = ok, -1 = existed , not changed
	// The pair::second element in the pair is set to true if a new element was inserted or false if an element with the same value existed.
}


int CDetElement::disconnect(int element_id)
{
	
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_top.begin(); ielem!=m_connectors_top.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			m_connectors_top.erase(ielem);
			return 0;
		}
	}
	
	for (map <int, CDetElement*> ::iterator ielem = m_connectors_bottom.begin(); ielem!=m_connectors_bottom.end(); ++ielem) {
		if(ielem->second->get_id() == element_id) {
			m_connectors_bottom.erase(ielem);
			return 0;
		}
	}
	return -1;
}


const void CDetElement::print() const 
{  
	cout <<  "id:" << get_id() << " name:" << name();

	cout << " top[" ;
	for (map <int, CDetElement*> ::const_iterator ielem = m_connectors_top.begin(); ielem!=m_connectors_top.end(); ++ielem) {
		cout << " (" << ielem->first << " ";
		ielem->second->print();
		cout << ") ";
	}
	cout << "] ";
	
	cout << " bottom[" ;
	for (map <int, CDetElement*> ::const_iterator ielem = m_connectors_bottom.begin(); ielem!=m_connectors_bottom.end(); ++ielem) {
		cout << " (" << ielem->first << " ";
		ielem->second->print();
		cout << ") ";
	}
	cout << "] ";

}

