/*
 *  CDetFec.h
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CDetFec_h
#define CDetFec_h

#include <string>
#include <vector>

#include "CDetElement.h"

class CDetChip;
//class CDetAdapterBoard;

class CDetFec : public CDetElement {
public:
	CDetFec();
	CDetFec(int i, const std::string n = "");
	virtual ~CDetFec();
	
	CDetFec& operator=(const CDetElement& rhs);
	CDetFec& operator=(const CDetFec& rhs);

	
	virtual void connect(CDetElement* el);
	void ipaddress(std::string ip) { m_ipaddress_string = ip;}
	const std::string ipaddress() const { return m_ipaddress_string;}
	
	size_t number_of_chips() const { return m_connectors_bottom.size(); }
	
	CDetChip* find_chip(int chip_id) ;
//	void chip_connect(int chip_id, CDetAdapterBoard* el);
//	void chip_disconnect(int chip_id);
	
	const void print() const;
	
private:

	std::string m_ipaddress_string;
	long m_ipaddress;
};

#endif
