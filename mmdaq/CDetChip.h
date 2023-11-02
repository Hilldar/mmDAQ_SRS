/*
 *  CDetChip.h
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/20/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CDetChip_h
#define CDetChip_h

#include "CDetElement.h"

#include <cassert>
#include <string>
#include <vector>
#include <functional>


class CDetAdapterBoard;

class CDetChip : public CDetElement {
	
public:
	CDetChip(int i, int chipid, const std::string n = 0);
	CDetChip(const CDetChip& rhs);
	virtual ~CDetChip();
	CDetChip& operator=(const CDetChip& c);
	CDetChip& operator=(const CDetElement& rhs);

	
	const void print() const;
//	void disconnect() ;
	const bool isconnected() const ;
	const int chipId() const { return m_chip_id;}
	
	void connect_fec( CDetElement* elem);
	void disconnect_fec();
	const CDetElement* const fec() const;
//	const int get_chamber_strip(int chNum); //test

   bool isequal_chipid(int chipId) const { return chipId == m_id;}

private:
	std::string m_serial;
	int m_chip_id; // aka apvId
	
   
public:
	//CDetElement* m_connected_to; //connected to AB or CHamber
   
   class ChipIdLess : public std::binary_function<const CDetChip* , const CDetChip* , bool>
   {
   public:
      bool operator()(const CDetChip* lhs, const CDetChip* rhs) const {
         return (lhs->m_chip_id < rhs->m_chip_id);
      }
   };
};

#endif
