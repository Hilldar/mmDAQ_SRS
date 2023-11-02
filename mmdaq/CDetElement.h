/*
 *  CDetElement.h
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 *  base class for detector geometry -all elemets are derived from this
 *  it has bottom and top connectors for attaching other elements
 */
#ifndef CDetElement_h
#define CDetElement_h

#define CONNECT_BOTTOM 0
#define CONNECT_TOP 1

#include "MBtools.h"

#include <string>
#include <vector>
#include <map>

//PM: connectors are owned by CDetElement

class CDetElement;

class CDetElement {
	
public:	
	typedef enum {facing_down = 0, facing_up = 1} direction_t;

public:	
	CDetElement();
	CDetElement(int i, const std::string n);
	CDetElement(const CDetElement &e);
	virtual ~CDetElement();
	virtual CDetElement& operator=(const CDetElement& c);

	virtual CDetElement* find(int element_id);
	virtual CDetElement* find(std::string element_name);
	virtual int find_connector(int element_id);
	
	//getters
	virtual const int get_id()           const { return m_id; };
	virtual const std::string name() const { return m_name;};
	virtual const void print()       const ;
	virtual std::map <int, CDetElement*>* top_connectors()    { return  &m_connectors_top;}
	virtual std::map <int, CDetElement*>* bottom_connectors() { return  &m_connectors_bottom;}
	
   std::string get_full_name() const
   {
      if (m_parent && m_parent != this) {
         return m_parent->get_full_name() + "-" + m_name;
      }
      return m_name;
   }
   
	//settters
	virtual void id(int n)                 { m_id = n; };
	virtual void name(const char* newname) { m_name = newname;};
	virtual int connect(CDetElement* el , bool top_face = true) ;
	virtual int disconnect(int element_id);
	
	virtual void parent(CDetElement* el) { m_parent = el;}
	virtual CDetElement* parent() { return m_parent;}
	
protected:
	IDList IDs;
	int m_id;
	std::string m_name;
	CDetElement* m_parent;
	std::map <int, CDetElement*> m_connectors_bottom; //connectors are owned by CDetElement
	std::map <int, CDetElement*> m_connectors_top;
	
};

#endif
