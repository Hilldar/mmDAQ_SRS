/*
 *  CDetChamber.cpp
 *  test_geometry
 *
 *  Created by Marcin Byszewski on 1/21/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CDetChamber.h"
#include "CDetReadout.h"
#include "CConfiguration.h"
#include "CMMEvent.h"
#include "MBtools.h"

#include <TH1.h>
#include <TH2.h>

#include <iostream>
#include <map>

using std::cout;
using std::endl;
using std::cerr;
using std::map;

CDetChamber::CDetChamber(int i,std::string n)
: CDetElement(i,n), m_strips(), m_chips_on_strips()
{
	
}

CDetChamber::~CDetChamber()
{
	FreeClear(m_strips);

}


CDetChamber& CDetChamber::operator=(const CDetElement& rhs)
{
	if (this != &rhs) {
		CDetElement::operator=(rhs);
	}
	return *this;
}

CDetChamber& CDetChamber::operator=(const CDetChamber& rhs)
{
	if (this != &rhs) {
		CDetElement::operator=(rhs);
		// assign CDetChamber members
		
	}
	return *this;
}


int CDetChamber::connect(CDetElement* el)//, const  std::string strips_name) 
{
	CDetElement::connect(el, CONNECT_TOP);
/*
	//TODO: what plane, strips (X,Y)
	CDetElement* strips = find_strips(strips_name);
	if (strips) {
		m_chips_on_strips[strips->id()].push_back(el->id());
		CDetElement::connect(el, CONNECT_TOP);
		return 0;
	}
	return -1;*/
	return 0;
}


void CDetChamber::add_strips(CDetElement* el)
{
	m_strips.push_back(el);
}


CDetElement* CDetChamber::find_strips(std::string strips_name) const 
{
	for (std::vector <CDetElement*> ::const_iterator it = m_strips.begin(); it != m_strips.end(); ++it) {
		if ((*it)->name() == strips_name) {
			return *it;
		}
	}
	return 0;
}


const void CDetChamber::print() const 
{

	cout << "Chamber name:" << m_name << " id:" << get_id()  <<" ,number of chips:" << number_of_chips() << endl;;
	for (map <int, CDetElement*> ::const_iterator it = m_connectors_top.begin(); it!=m_connectors_top.end(); ++it) {
		cout << "top C"<<it->first << " " << it->second->name() << " " << it->second->get_id() << endl;
	}
	for (map <int, CDetElement*> ::const_iterator it = m_connectors_bottom.begin(); it!=m_connectors_bottom.end(); ++it) {
		cout << "btm C"<<it->first << " " << it->second->name() << " " << it->second->get_id() << endl;
	}
	
}


const CDetAbstractReadout* CDetChamber::get_readout_for_chipId(int chipId) const
{
   std::vector<CDetAbstractReadout*> ::const_iterator found = std::find_if(m_readouts.begin(), m_readouts.end(), 
                                                                           std::bind2nd( std::mem_fun(&CDetAbstractReadout::has_chip) , chipId));
   if (found != m_readouts.end()) {
      return *found;
   }   
   
//   for (std::vector<CDetAbstractReadout*> ::const_iterator ird = m_readouts.begin(); ird != m_readouts.end(); ++ird) {
//      if ((*ird)->get_id() == chipId ) {
//         return *ird;
//      }
//   }
   return 0;
}


void CDetChamber::create_hitmap(CConfiguration* config, CMMEvent* mmevt, std::vector<TH1*>& hitmaps)
{
   
   //create 2D histogram wth hitpoints
   TH2F* hist = new TH2F();
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
   hist->SetBit(TH1::kCanRebin);
#else
   hist->SetCanExtend(TH1::kXaxis);
#endif
   size_t number_of_readouts = m_readouts.size();
   if (number_of_readouts < 1) {
      std::cerr << "CDetChamber::create_hitmap() no readouts in chamber " << m_name << std::endl;
      return;
   }
   else if (number_of_readouts == 1) {
      std::vector<CDetAbstractReadout*>::const_iterator ird1 = m_readouts.begin();
      const std::vector<CDetChip*>& chips1 = (*ird1)->get_chips();
//      std::cout << m_name << "-" << (*ird1)->name() << " has chips1.size()=" << chips1.size() << std::endl;
      //loop on chips, get events
      for (std::vector<CDetChip*> :: const_iterator ichip1 = chips1.begin(); ichip1 != chips1.end(); ++ichip1) {
         const CApvEvent* apvevt1 = mmevt->get_apv_event( (*ichip1)->chipId() );
         if (!apvevt1) {
//            std::cout << "CDetChamber::create_hitmap: no apvevt1 " << std::endl;
            continue;
         }
         const std::vector<CStripDataReduced>& data1 = apvevt1->apv_reduced_data();
         fill_hit_positions(config, hist, *ird1, data1);
      }//for ichip1
//      std::cout << m_name << " hist entries " << hist->GetEntries() << std::endl;
      hitmaps.push_back(hist);
      return;
   }
   
   // number_of_readouts>1
   for (std::vector<CDetAbstractReadout*>::const_iterator ird1 = m_readouts.begin(); ird1 != m_readouts.end(); ++ird1) {
      const std::vector<CDetChip*>& chips1 = (*ird1)->get_chips();
      
      for (std::vector<CDetAbstractReadout*>::const_iterator ird2 = ird1; ird2 != m_readouts.end(); ++ird2) {
         if (ird1 == ird2) {
            continue;
         }
         const std::vector<CDetChip*>& chips2 = (*ird2)->get_chips();
         
         //loop on chips, get events
         for (std::vector<CDetChip*> :: const_iterator ichip1 = chips1.begin(); ichip1 != chips1.end(); ++ichip1) {
            const CApvEvent* apvevt1 = mmevt->get_apv_event( (*ichip1)->chipId() );
            if (!apvevt1) {
               continue;
            }
            const std::vector<CStripDataReduced>& data1 = apvevt1->apv_reduced_data();
            for (std::vector<CDetChip*> :: const_iterator ichip2 = chips2.begin(); ichip2 != chips2.end(); ++ichip2) {
               const CApvEvent* apvevt2 = mmevt->get_apv_event( (*ichip2)->chipId() );
               if (!apvevt2) {
                  continue;
               }
               const std::vector<CStripDataReduced>& data2 = apvevt2->apv_reduced_data();
               fill_hit_positions(config, hist, *ird1, data1, *ird2, data2);
//               std::cout << m_name << " hist entries " << hist->GetEntries() << std::endl;
            } //for ichip2
         }//for ichip1
         
      }// for ird2
   } // for ird1
   hitmaps.push_back(hist);
}


void CDetChamber::fill_hit_positions(CConfiguration* config, TH2F* hist, 
                                     const CDetAbstractReadout* rd1, const std::vector<CStripDataReduced>& data1, 
                                     const CDetAbstractReadout* rd2, const std::vector<CStripDataReduced>& data2)
{   
   for (std::vector<CStripDataReduced>::const_iterator id1 = data1.begin(); id1 != data1.end(); ++id1) {
      int strip1 = config->strip_from_channel(id1->get_channel_id());
      for (std::vector<CStripDataReduced>::const_iterator id2 = data2.begin(); id2 != data1.end(); ++id2) {
         int strip2 = config->strip_from_channel(id2->get_channel_id());
         TVector2 hit = get_strip_crossing(rd1, strip1, rd2, strip2);
//         std::cout << " CDetChamber::fill_hit_positions() strip1="  << strip1<< " strip2=" 
//         << strip2 << " hit=" << hit.X() <<"," << hit.Y()<<std::endl;
         hist->Fill(hit.X(), hit.Y());
      }
   }
}

void CDetChamber::fill_hit_positions(CConfiguration* config, TH2F* hist, 
                                     const CDetAbstractReadout* rd1, const std::vector<CStripDataReduced>& data1)
{
   for (std::vector<CStripDataReduced>::const_iterator id1 = data1.begin(); id1 != data1.end(); ++id1) {
      int strip1 = config->strip_from_channel(id1->get_channel_id());
         TVector2 hit = get_strip_crossing(rd1, strip1, NULL, 0);
//      std::cout << " CDetChamber::fill_hit_positions() strip1="  << strip1<< " strip2=NA" 
//      << " hit=" << hit.X() <<"," << hit.Y()<<std::endl;
         hist->Fill(hit.X(), hit.Y());
   }
}


TVector2 CDetChamber::get_strip_crossing(const CDetAbstractReadout* rd1, int strip1, const CDetAbstractReadout* rd2, int strip2)
{
   return rd1->get_strip_crossing(strip1, rd2, strip2);
}


