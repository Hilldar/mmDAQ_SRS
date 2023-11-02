/*
 *  CPublisher.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/21/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

//proj's
#include "CPublisher.h"
#include "CEvent.h"
#include "CMMEvent.h"
#include "CMutex.h"

#include "CDetChamber.h"
#include "CDetReadout.h"
#include "CDetChip.h"

#include "CConfiguration.h"
#include "CRootWriter.h"


//root
#include <TCondition.h>
#include <TH1.h>
#include <TH2.h>

//boost
#include <boost/bind.hpp>

//sys
#include <cassert>
#include <iostream>
#include <list>
#include <vector>
#include <string>


#define NUMBER_HIST_STAT 3

using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::vector;
using std::string;

CPublisher::CPublisher()
      : m_config(0), m_writer(0), m_event(0), m_statistics_event(0), m_cond_newrequest(0), m_cond_dataready(0),
      m_mx_processed(0), m_mx_published(0), m_published(), m_published_udp()
{

}


CPublisher::~CPublisher()
{
   for (std::map<int, std::vector<TH1*> > :: iterator it = m_published.begin(); it != m_published.end(); ++it) {
      FreeClear(it->second);
   }
  
   while(!m_published_udp.empty()) {
      delete m_published_udp.begin()->second;
      m_published_udp.erase(m_published_udp.begin());
   }
   

   
   delete m_event; m_event = 0;

}

//void CPublisher::request_clear()
//{
//	m_request_clear = true;
//	m_cond_newrequest
//}

void CPublisher::clear()
{
   for (std::map<int, std::vector<TH1*> > :: iterator it = m_published.begin(); it != m_published.end(); ++it) {
      FreeClear(it->second);
   }
   m_published.clear();
   
   while(!m_published_udp.empty()) {
      delete m_published_udp.begin()->second;
      m_published_udp.erase(m_published_udp.begin());
   }
   
   //delete m_event; m_event = 0;
}




void CPublisher::detach(CRootWriter* writer)
{
   assert(m_writer == writer);
   m_writer = 0;
}


void CPublisher::detach(CConfiguration* config)
{
   assert(m_config == config);
   m_config = 0;
}


void CPublisher::detach_signal_request(TCondition* cond)
{
   assert(m_cond_newrequest == cond);
   m_cond_newrequest = 0;
}

void CPublisher::detach_signal_newdata(TCondition* cond)
{
   assert(m_cond_dataready == cond);
   m_cond_dataready = 0;
}

void CPublisher::detach_mutex_processed_data(CMutex* mut)
{
   assert(m_mx_processed == mut);
   //m_mx_processed->unlock();
   m_mx_processed = 0;
}

void CPublisher::detach_mutex_published_data(CMutex* mut)
{
   assert(m_mx_published == mut);
   //m_mx_processed->unlock();
   m_mx_published = 0;
}


int CPublisher::main(void*)
{

   assert(m_cond_newrequest);

   while (1) {

      //wait for requests
      // root signals

      int rc = m_cond_newrequest->Wait();  // waiting here
      //cout << "CPublisher::main(): exited wait() .." <<endl;

      if (rc < 0) {
         cerr << "CPublisher::main(): m_cond_newrequest.Wait() returned " << rc << endl;
      }

      bool new_event = false;
      //replace previous event data with data from CRootWriter
      if (m_writer) {
         //cout << "CPublisher::main(): now will m_mx_processed->lock()" <<endl;
         m_mx_processed->lock(thread_id() , thread_name() , "main() ,1");
         const CEvent* evt = m_writer->current_event();
//         const CMMEvent* mmevt = dynamic_cast<const CMMEvent*>(evt);

         if (m_event && evt 
	     //&& !mmevt->is_all_empty() 
	     && (evt->event_number() != m_event->event_number())) {
            new_event = true;
         }
         else if (!m_event) {
            new_event = true;
         }

         if (new_event) {
            delete m_event; m_event = 0;
            const CMMEvent* mmevt = dynamic_cast<const CMMEvent*>(evt);  //evt is not NULL when empty ??w
            if (mmevt) {
               m_event = new CMMEvent(*mmevt);		//fully fledged new deep copy
            }
         }
         //cout << "CPublisher::main(): now will m_mx_processed->unlock()" <<endl;
         m_mx_processed->unlock();
      }

      if (!new_event || !m_writer || !m_event) {
         continue;	//m_writer==NULL means DAQ is not running do nothing
      }


      //create published data
      //create histograms or just data in vectors

      split_mbt0_hack();
       
      
      
      //if (!m_event->is_bad()) {
         fill_statistics();
         
      	create_udp_data();
      	create_1d_data();
	create_2d_data();
      //}
      
      
      //TODO: prepare only that WHAT is requested
		
      //signal new data is ready
      m_cond_dataready->Signal();
      //cout << "CPublisher::main(): m_cond_dataready->Signal()" << endl;
   }
   return 0;
}

void CPublisher::split_mbt0_hack()
{
//   const std::vector<CDetChamber*>& chambers = m_config->defined_chambers();
//   for (std::vector<CDetChamber*>::const_iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {
//      if ((*ichamb)->name().compare(0,4,"MBT0") == 0 ) {
////         m_event->split_mbt0_hack();
//      }  
//   } 
}


void CPublisher::delete_histograms(std::vector<TH1*>& vec)
{
   FreeClear(vec);
//   while (!vec.empty()) {
//      TH1* h = vec.front();
//      vec.erase(vec.begin());
//      TH1F* h1 = dynamic_cast<TH1F*>(h);
//      TH2F* h2 = dynamic_cast<TH2F*>(h);
//      delete h1; h1 = 0;
//      delete h2; h2 = 0;
//   }
}



const vector<TH1*> CPublisher::get_published_data(int request_id) const
{

   std::map<int, std::vector<TH1*> > :: const_iterator data = m_published.find(request_id);
   if (data != m_published.end()) {
      return data->second;
   }
   return vector<TH1*>(0, 0L);
}

const std::map<int, TH1*>&  CPublisher::get_published_data_udp(int request_id) const
{
   return m_published_udp;
}


void CPublisher::clear_statistics() 
{
	FreeClear(m_published[PUBLISHER_REQUEST_ID_STATISTICS]); 
}


void CPublisher::init_statistics()
{
	CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
//   if (!mmevt) {
//      return;
//   }
	m_mx_published->lock(thread_id());
	
	clear_statistics();
	vector<TH1*>& histvec = m_published[PUBLISHER_REQUEST_ID_STATISTICS];
	
   
   const std::vector<CDetChamber*>& chambers = m_config->defined_chambers();
   for (std::vector<CDetChamber*>::const_iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {
      const std::vector<CDetAbstractReadout*>& readouts = (*ichamb)->get_readouts();
      
      // create histograms for each readout on each chamber
      for (std::vector<CDetAbstractReadout*>::const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
         histvec.push_back( (*ird)->create_stat_qmax() );
         histvec.push_back( (*ird)->create_stat_tqmax(mmevt) );
         histvec.push_back( (*ird)->create_stat_nqmax() );
      }
   }
   /*
	vector<string> chamber_names = m_config->defined_chamber_names();
   int n_chambers = chamber_names.size();
	for (int ii = 0; ii < n_chambers*NUMBER_HIST_STAT; ++ii) {
		const CDetChamber* chamber = m_config->chamber_by_name(chamber_names[ii/NUMBER_HIST_STAT]);
		if (!chamber) {
			continue;
		}
      std::pair<unsigned, unsigned> rng = m_config->defined_chamber_strip_ranges(chamber->get_id());
      int lostrip = rng.first ? rng.first : 1;
      int histrip = rng.second ? rng.second : 128;
		int timebin_count = mmevt->max_timebin_count();
      timebin_count = timebin_count ? timebin_count : 20;
		std::stringstream ss;

		TH1F* h0 = new TH1F();
		if (ii%NUMBER_HIST_STAT == 0) {
			ss << chamber->name() << "_qmax_strip";
			h0->SetBins(histrip - lostrip, lostrip, histrip);
		}
		else if (ii%NUMBER_HIST_STAT == 1) {
			ss << chamber->name() << "_tqmax";
			h0->SetBins(timebin_count, 0, timebin_count);
		}
		else if (ii%NUMBER_HIST_STAT == 2) {
			ss << chamber->name() << "_N_qmax";
			h0->SetBins(300, 0, 3000);
		}
		h0->SetName(ss.str().c_str());
		h0->SetTitle(ss.str().c_str());
		
		histvec.push_back(h0);
	}
	*/
   
	m_mx_published->unlock();
}



void CPublisher::fill_statistics()
{
	CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
   if (!mmevt || mmevt->is_all_empty()) {
      return;
   }
	
	if(m_published[PUBLISHER_REQUEST_ID_STATISTICS].empty())	{
		init_statistics(); //not initialised
	}; 
	
	m_mx_published->lock(thread_id());
	
	vector<TH1*>& histvec = m_published[PUBLISHER_REQUEST_ID_STATISTICS];
	
   const std::vector<CDetChamber*>& chambers = m_config->defined_chambers();
   for (std::vector<CDetChamber*>::const_iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {
      const std::vector<CDetAbstractReadout*>& readouts = (*ichamb)->get_readouts();
      
      // create histograms for each readout on each chamber
      for (std::vector<CDetAbstractReadout*>::const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
//         std::string ssqmax, sstqmax, ssnqmax;
         
         //get histogramns to fill
         std::string ssqmax((*ichamb)->name() + (*ird)->name() + "_qmax_strip");
         std::string sstqmax((*ichamb)->name() + (*ird)->name() + "_tqmax_strip");
         std::string ssnqmax((*ichamb)->name() + (*ird)->name() + "_N_qmax");
         TH1* hqmax = 0;
         TH1* htqmax = 0;
         TH1* hnqmax = 0;
         for (std::vector<TH1*>::iterator ih = histvec.begin(); ih != histvec.end(); ++ih) {
            if ( std::string((*ih)->GetName()) == ssqmax ) {
               hqmax = *ih;
            }
            if ( std::string((*ih)->GetName()) == sstqmax ) {
               htqmax = *ih;
            }
            if ( std::string((*ih)->GetName()) == ssnqmax ) {
               hnqmax = *ih;
            }            
         }
         if (!hqmax || !htqmax || !hnqmax) {
            continue;
         }
         
         (*ird)->fill_stat(hqmax, htqmax, hnqmax, mmevt, m_config);
      }
   }

   
   /*
	vector<string> chamber_names = m_config->defined_chamber_names();
   int n_chambers = chamber_names.size();
	vector<int> chamber_maxq(n_chambers,0);
	vector<int> chamber_maxqt(n_chambers,0);
	
	//loop events, fill data
   int ii = 0;
   const std::list< CEvent* > events = mmevt->apv_events();
   for (list <CEvent*>::const_iterator ievt = events.begin(); ievt != events.end(); ++ievt, ++ii) {
      CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievt);
      if (!apvevt) {
         continue;
      }
      const std::map<int, std::vector<int16_t> > * const data2d = apvevt->apv2d_data();
      for (std::map<int, std::vector<int16_t> > :: const_iterator ichan = data2d->begin(); ichan != data2d->end(); ++ichan) {
         int stripNo = m_config->strip_from_channel(ichan->first);
			if ((stripNo - 1) < 0)
            continue; // check for unmapped channels
         //max values
         std::vector<int16_t>::const_iterator imax = max_element(ichan->second.begin(), ichan->second.end());
         int16_t valqmax = *imax;
         int16_t tbqmax  = imax - ichan->second.begin();
         int apv_id   = CApvEvent::chipId_from_chId(ichan->first); //int apv_id   = (ichan->first >> 8) & 0xFF;

         //TODO: THIS IS BAD id() may not be consecutive!!!!
			int ichamber = m_config->chamber_id_by_chip(apv_id) - 1;
         if (ichamber < 0) {
//            cout << "CPublisher::fill_statistics() : unknown apv_id=" << apv_id << endl;
            continue;
         }
			
			//cout << "ichamber=" << ichamber << endl; 
         TH1* h0 = dynamic_cast <TH1*> (histvec[ichamber*NUMBER_HIST_STAT]);
         //TH1* h1 = dynamic_cast <TH1*> (histvec[ichamber*NUMBER_HIST_STAT + 1]);
			//TH1* h2 = dynamic_cast <TH1*> (histvec[ichamber*NUMBER_HIST_STAT + 2]);
			
			if(h0) h0->Fill(stripNo, valqmax);
			//if(h1) h1->Fill(tbqmax); // all strips tqmax
			
			if (valqmax > chamber_maxq[ichamber]) {
				chamber_maxq[ichamber] = valqmax;
				chamber_maxqt[ichamber] = tbqmax;
			}
			
			//chamber_maxq[ichamber] = chamber_maxq[ichamber]>valqmax ? chamber_maxq[ichamber] : valqmax;
			
			
		}//for ichan
		
		
   }//for ievt

	for (int ichamber = 0 ; ichamber < n_chambers; ++ichamber) {
		TH1* h1 = dynamic_cast <TH1*> (histvec[ichamber*NUMBER_HIST_STAT + 1]);
		if(h1) h1->Fill(chamber_maxqt[ichamber]);
		TH1* h2 = dynamic_cast <TH1*> (histvec[ichamber*NUMBER_HIST_STAT + 2]);
		if(h2) h2->Fill(chamber_maxq[ichamber]);
	}
    */
   
	m_mx_published->unlock();
	
}



void CPublisher::create_udp_data()
{
   CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
   if (!mmevt) {
      return;
   }

	m_mx_published->lock(thread_id());
   FreeClear(m_published[PUBLISHER_REQUEST_ID_UDP]);
   
   
   while(!m_published_udp.empty()) {
      delete m_published_udp.begin()->second;
      m_published_udp.erase(m_published_udp.begin());
   }
   
   //delete_histograms(m_published[0]);

//   std::map<int, TH1*> pub;


   const std::list< CEvent* > events = mmevt->apv_events();
   size_t max_allowed_evts = m_config->defined_chip_names().size();
   if (events.size() > max_allowed_evts) {
      std::cout << "Publisher: too many chips.  skip."  << std::endl;
      m_mx_published->unlock();
      return;
   }

   //TODO: assign chip id numbers to data
   
   std::map<int, TH1F*> all_chip_map;

   int ii = 0;
   for (list <CEvent*>::const_iterator ievt = events.begin(); ievt != events.end(); ++ievt, ++ii) {
      CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievt);
      if (!apvevt) {
         continue;
      }
      int apv_id = apvevt->apv_no(); // 0-15
      
      
      bool isbad = apvevt->is_bad();
      const vector<int16_t>* const udpdata = apvevt->udp_data();
      TH1F* h = new TH1F();
		std::stringstream ss;
		ss << m_config->defined_chip_names()[ii] << " " <<  "(" << m_event->event_number() << ")";
      h->SetTitle(ss.str().c_str());
      h->SetBins((int)udpdata->size(), 0, (int)udpdata->size());
      int ipoint = 0;
      for (vector<int16_t>::const_iterator idatum = udpdata->begin(); idatum != udpdata->end(); ++idatum, ++ipoint) {
         h->Fill(ipoint, *idatum);
      }
      if (isbad) {
         h->SetLineColor(kRed);
         h->SetMarkerColor(kRed);
	
	
      }
      all_chip_map.insert(std::pair<int, TH1F*>(apv_id, h));
      
   }
   //TODO: temporarily req_id for udp is fixed at 0 (ui tab number)

   //assign to pub 0-15, empty ones are nullptr
//   for (size_t ii = 0; ii < 16; ++ii) {
//      pub.push_back(all_chip_map[ii]);
//   }
   
   
//   m_published[PUBLISHER_REQUEST_ID_UDP] = pub;
   m_published_udp.insert(all_chip_map.begin(), all_chip_map.end());
   m_mx_published->unlock();
}


void CPublisher::create_1d_data()
{
   CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
   if (!mmevt) {
      return;
   }
   m_mx_published->lock(thread_id());
   //   delete_histograms(m_published[1]);
   FreeClear(m_published[PUBLISHER_REQUEST_ID_1D]);
   std::vector<TH1*> pub;

   const std::vector<CDetChamber*>& chambers = m_config->defined_chambers();
   for (std::vector<CDetChamber*>::const_iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {      
      const std::vector<CDetAbstractReadout*>& readouts = (*ichamb)->get_readouts();
      // create histograms for each readout on each chamber
      for (std::vector<CDetAbstractReadout*>::const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
         std::vector<TH1*> hvec = (*ird)->create_th1_qt(mmevt);

         pub.insert(pub.end(), hvec.begin(), hvec.end());
      }
   }

   //TODO: temporarily req_id for udp is fixed at 1 (ui tab number)
   m_published[PUBLISHER_REQUEST_ID_1D] = pub;
   m_mx_published->unlock();
}


void CPublisher::create_2d_data()
{

	CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
   if (!mmevt) {
      return;
   }
   m_mx_published->lock(thread_id());
   FreeClear(m_published[PUBLISHER_REQUEST_ID_2D]);
   std::vector<TH1*> pub;
	
   const std::vector<CDetChamber*>& chambers = m_config->defined_chambers();
   for (std::vector<CDetChamber*>::const_iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {
      const std::vector<CDetAbstractReadout*>& readouts = (*ichamb)->get_readouts();
      
      // create histograms for each readout on each chamber
      for (std::vector<CDetAbstractReadout*>::const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
         pub.push_back( (*ird)->create_th2_qt(mmevt) );
      }
   }
 	
	m_published[PUBLISHER_REQUEST_ID_2D] = pub;
   m_mx_published->unlock();
}


/**
 based on config create hitmaps for 2d chambers
 */

void CPublisher::create_hitmap_data()
{
   
   m_mx_published->lock(thread_id());

   CMMEvent* mmevt = dynamic_cast<CMMEvent*>(m_event);
   if (!mmevt) {
      return;
   }
   FreeClear(m_published[PUBLISHER_REQUEST_ID_2DHITMAP]);
   std::vector<TH1*> pub;
	
   const vector<CDetChamber*>& chambers = m_config->defined_chambers();
   std::for_each(chambers.begin(), chambers.end(), boost::bind(&CDetChamber::create_hitmap, _1, m_config, mmevt, boost::ref(pub) ));
   
   ///////////////////////////////////
	m_published[PUBLISHER_REQUEST_ID_2DHITMAP] = pub;
   m_mx_published->unlock();
   
}

