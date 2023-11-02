/*
 *  CDetChamber.h
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/21/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CDetChamber_h
#define CDetChamber_h


#include "CDetElement.h"
//#include "CApvEvent.h"
#include <TVector2.h>

#include <string>
#include <vector>
#include <map>

class CDetAbstractReadout;
class CMMEvent;
class CStripDataReduced;
class CConfiguration;

class TH1;
class TH2F;

class CDetChamber : public CDetElement {
public:
	
	CDetChamber(int i,std::string n);
	~CDetChamber();
	CDetChamber& operator=(const CDetElement& rhs);
	CDetChamber& operator=(const CDetChamber& rhs);

	
	size_t number_of_chips() const { return m_connectors_top.size(); }
	const void print() const ;
	virtual int connect( CDetElement* el/*, const std::string stripsname*/) ;

	void add_strips(CDetElement* el);
	CDetElement* find_strips(std::string name) const ;
	void add_readout(CDetAbstractReadout* readout) {m_readouts.push_back(readout); }
   const std::vector<CDetAbstractReadout*>& get_readouts() const { return m_readouts;} 
   const CDetAbstractReadout* get_readout_for_chipId(int chipId) const;
   
   void create_hitmap(CConfiguration* config, CMMEvent* mmevt, std::vector<TH1*>& hitmaps);
protected:
   void fill_hit_positions(CConfiguration* config, TH2F* hist, 
                           const CDetAbstractReadout* rd1, const std::vector<CStripDataReduced>& data1, 
                           const CDetAbstractReadout* rd2, const std::vector<CStripDataReduced>& data2);
   void fill_hit_positions(CConfiguration* config, TH2F* hist, 
                           const CDetAbstractReadout* rd1, const std::vector<CStripDataReduced>& data1);   
   inline TVector2 get_strip_crossing(const CDetAbstractReadout* rd1, int strip1, const CDetAbstractReadout* rd2, int strip2);
protected:
	//chamber will have planes and strips as members not "connected" to connectors

	//std::vector <CDetElement*> m_planes;
	//std::map <int, std::vector<int>> m_chips_on_plane;  // id_plane -> id_chips

   
	std::vector <CDetElement*> m_strips;
	std::map <int, std::vector<int> > m_chips_on_strips; // id_plane -> id_chips
   std::vector<CDetAbstractReadout*> m_readouts;
	
};

#endif

