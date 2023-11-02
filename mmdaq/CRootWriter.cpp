/*
 *  CRootWriter.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/9/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CListener.h"
#include "CPublisher.h"
#include "CMMDaq.h"
#include "CThreadRoot.h"
#include "CMutex.h"
#include "CGuiTabContents.h"
#include "CGuiDetConfig.h"
#include "CGuiUdpRawData.h"


#include "CDetChip.h"

#include "CUserInterface.h"
#include "CRootWriter.h"
#include "CEvent.h"
#include "CApvEvent.h"
//#include "CApvEventDecoder.h"
#include "CConfiguration.h"
#include "CDetChamber.h"
#include "CDetReadout.h"

#include "MBtools.h"

#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"
#include "TEnv.h"

//#include <arpa/inet.h>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>


//#include <pthread.h>
#include <string>
#include <sstream>
#include <sys/time.h>     /* struct timeval definition           */
#include <unistd.h>       /* declaration of gettimeofday()       */

#ifndef MAXNUMOF_RAW_TREEBRANCHES
#define MAXNUMOF_RAW_TREEBRANCHES 11
#endif

#ifndef MAXNUMOF_PED_TREEBRANCHES
#define MAXNUMOF_PED_TREEBRANCHES 13
#endif

#ifndef MAXNUMOF_DATA_TREEBRANCHES
#define MAXNUMOF_DATA_TREEBRANCHES 5
#endif

#ifndef MAXNUMOF_INFO_TREEBRANCHES
#define MAXNUMOF_INFO_TREEBRANCHES 5
#endif


using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
//using std::pair;
using std::map;
//using std::clock;
using std::hex;
using std::dec;
using namespace std;

CRootWriter::CRootWriter(CMMDaq* daq, CConfiguration* config, std::list <CMMEvent*>* eventcontainer,
			 CMutex* publishing_mutex, CMutex* processed_data_mutex, pthread_cond_t& action_event_processed,
                         TCondition* cond_proc, bool save_data)

      : CThreadRoot(),// m_daq(daq), 
     m_error(0), m_daq(daq), m_config(config), m_mm_events_decoded(eventcontainer), 
		m_mm_events_local(0,0L),
      m_pedestals_mmevent(CMMEvent(m_config->run_number(), 0)),
		m_bytes_written(0L),
		//m_statistics_mmevent(CMMEvent(m_config->run_number(), 0, 0)),
      m_publishing_mutex(publishing_mutex), 
		m_processed_data_mutex(processed_data_mutex), 
		m_action_event_processed(&action_event_processed),
      m_condition_processed(cond_proc),
      m_rootfile(0), m_rawtree(0), m_pedtree(0), m_datatree(0), m_infotree(0),
      m_rawbranches(MAXNUMOF_RAW_TREEBRANCHES, 0L), m_pedbranches(MAXNUMOF_PED_TREEBRANCHES, 0L), 
		m_databranches(MAXNUMOF_DATA_TREEBRANCHES, 0L), m_infobranches(MAXNUMOF_INFO_TREEBRANCHES, 0L), 
      m_event_counter(0), m_event_counter_pedestals(0L), m_event_counter_pedestals_done(0L), m_pedestal_counts_per_save(m_config->pedestal_events_per_save()), m_event_has_data(false), m_save_data_flag(save_data),
      m_runnumber(m_config->run_number()),  m_number_of_raw_tree_branches(0), m_number_of_ped_tree_branches(0), m_number_of_data_tree_branches(0),
      m_apv_evt(0), m_time_s(0), m_time_us(0), m_apv_fec(), m_apv_id(), m_apv_ch(), m_mm_id(),	m_mm_readout(0), m_mm_strip(),	m_apv_q(0, vector <short>()), 
		m_apv_presamples(0), m_apv_pedmean(), m_apv_pedsigma(), m_apv_pedstd(),	m_apv_qmax(), m_apv_tbqmax(),
		m_info_comment(""), m_info_zero_factor(m_config->sigma_cut()),

      m_pedestals_mean(m_config->pedestal_mean_map()->begin(), m_config->pedestal_mean_map()->end()),
      m_pedestals_sigma(),
      m_pedestals_stdev(m_config->pedestal_stdev_map()->begin(), m_config->pedestal_stdev_map()->end()),
      m_channel_map(m_config->channel_map()->begin(), m_config->channel_map()->end()),
      m_internal_queue_size(0), m_internal_queue_decoded(0)
{
   //std::string name;

	std::stringstream ssname;
	ssname << m_config->write_path() << "run" << NumberToString(m_runnumber) + ".root";
	m_mm_events_local.clear();
  // name = "run" + NumberToString(m_runnumber) + ".root";
   root_open_file(ssname.str());

}


CRootWriter::~CRootWriter()
{
   CThreadRoot::lock_mutex();
   if (m_rootfile && m_rootfile->IsOpen()) {
      m_rootfile->Close();
   }
   CThreadRoot::unlock_mutex();
   delete m_rootfile;
   m_rootfile = 0;
   //	for (vector<TBranch*> :: iterator ibr = m_rawbranches.begin(); ibr != m_rawbranches.end(); ++ibr) {
   //		(*ibr) = 0;
   //	}
  // TThread::Ps();
}



int CRootWriter::main(void*)
{
   struct timeval  now;            /* time when we started waiting        */
   struct timespec timeout;        /* timeout value for the wait function */
   set_cancel_state_on();

   //cout << "CRootWriter::main(void*)" << endl;

   /* make sure we're in asynchronous cancelation mode so   */
   /* we can be canceled even when blocked on reading data. */
   //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

   // lock data on init ....
   //copy m_mm_events_decoded to m_mm_events_local and remove data

	
//	CThreadRoot::lock_mutex();
	//initial writing of data: pedestals, runinfo
	if (m_save_data_flag && m_config->run_type() == CConfiguration::runtypePhysics ) {
		//add pedestal data
		const CMMEvent* const loaded_pedestals = m_config->pedestal_event();
		if (loaded_pedestals) {
			long nbytes = root_fill_tree_pedestals(loaded_pedestals);
			m_bytes_written += nbytes;
		}
		
	}
//	CThreadRoot::unlock_mutex();
	
	//main loop here
   m_processed_data_mutex->lock(thread_id() , thread_name() , "main() ,1");

   while (1) {

      thread_cancel_point();

      gettimeofday(&now, NULL);
      // prepare timeout value. Note that we need an absolute time.
      timeout.tv_sec = now.tv_sec + 1;
      timeout.tv_nsec = now.tv_usec * 1000; /* timeval uses micro-seconds.         */

      int rc = 0;
      //if (!m_decoder->event_ready_check() ) {
      /* remember that pthread_cond_timedwait() unlocks the mutex on entrance */


//      rc = pthread_cond_timedwait(m_action_event_processed, mx, &timeout);
      //}

      rc = m_processed_data_mutex->timedwait(m_action_event_processed, &timeout, thread_name());

      if (!m_root_thread) {
         break;
      }
      m_internal_queue_decoded = m_mm_events_decoded->size();

      switch (rc) {
         case 0:  /* we were awakened due to the cond. variable being signaled */
            // the mutex was now locked again by pthread_cond_timedwait.
            // we were signaled that decoder prepared an event or queue not empty
				
				clear_local_events();
				//that should be empty anyway ?
				
				
//            for (std::list <CMMEvent*> :: iterator ievent = m_mm_events_decoded->begin(); ievent != m_mm_events_decoded->end(); ++ievent) {
//               m_mm_events_local.push_back(*ievent);
//            }
//            m_mm_events_decoded->clear();
            m_mm_events_local.splice(m_mm_events_local.end(), *m_mm_events_decoded);

            
            //TODO: this here?		m_receiver->event_ready_unset();
            //temporarly unlock m_receiver_data for receiver access while processing data
            //temporarly disable process cancelation while we write data

            m_processed_data_mutex->unlock();
				
            //CThread::unlock_mutex(m_processed_data_mutex);
            //int thread_old_cancel_state;
            //pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &thread_old_cancel_state);

            m_internal_queue_size = m_mm_events_local.size();
            for (std::list <CMMEvent*> :: iterator ievent = m_mm_events_local.begin(); ievent != m_mm_events_local.end(); ++ievent) {
					thread_cancel_point();
					++m_event_counter;
					
					int cont=0;
					const std::list <CEvent*>& list_apv_events = (*ievent)->apv_events();
					
					bool all_empty = true;
					bool all_good = true;
					for (std::list	<CEvent*> :: const_iterator ievt = list_apv_events.begin(); ievt != list_apv_events.end(); ++ievt) {
						CApvEvent* apvevt = dynamic_cast<CApvEvent*> (*ievt);
						if (!apvevt) {
							continue;
						}
						if (!apvevt->is_empty() ) {
							all_empty = false;
						}
						if ( apvevt->is_bad() ) {
							all_good = false;
						}
					}
					
					//all_good shouldd be the same as (ievent)->bad()
					if ((*ievent)->is_bad() || all_empty || !all_good) {
					
						
		/*				//pause DAQ
						m_daq->pause_receiver(kTRUE);	
						//try resetting with scripts
						if(cont=0){
						system("../../Scripts/setup_mmdaq.sh");
						sleep(1);
						system("../../Scripts/CScript/physicsRun.sh");
						cont= 1;
						continue;
						}else{	
						//take the time of error and write to file
						system("../../Scripts/setup_mmdaq.sh");
						//system("../../Scripts/CScript/stop.sh");
						char time_char[100];
						std::time_t t= std::time(NULL);
						std::strftime(time_char, sizeof(time_char), "system reboot at %d/%m/%Y %H:%M:%S\n", std::localtime(&t));
						ofstream myfile;
						myfile.open("../../../shared/reboot_Log.txt", ios::app);
						myfile << time_char;
						myfile.close();
						system("sudo mount -t vboxsf vbox_shared /home/physics/shared");
						//wait some time (optimized)
						sleep(15);
						cout<<"SRS rebooted. Can send settings now"<<endl;
						//configure electronics
						
						sleep(2);
						system("../../Scripts/CScript/physicsRun.sh");
						sleep(1);
						system("../../Scripts/CScript/stop.sh");
						sleep(1);
						//resume DAQ
						m_daq->pause_receiver(kFALSE);
						system("../../Scripts/CScript/start.sh");
						cont=0;	
						continue;
					
						}*/
					}
               
               switch ((*ievent)->event_type()) {
                  case CEvent::eventTypePhysics :
							process_physics_event(*ievent);
                     break;
                  case CEvent::eventTypePedestals:
                     process_pedestal_event(*ievent);
                     break;

                  default:
                     cerr << "CRootWriter::main(): Unhandled event type "<< (*ievent)->event_type() << endl;
                     break;
               }
               //delete *ievent; *ievent = 0;
            }
            
            //pthread_setcancelstate(thread_old_cancel_state, NULL);

            m_processed_data_mutex->lock(thread_id() , thread_name(), "main() , 2");
            //CThread::lock_mutex(m_processed_data_mutex); //lock back (pthread_cond_timedwait will unlock)

            break;
         default:        /* some error occurred (e.g. we got a Unix signal) */
            if (errno == ETIMEDOUT) { /* our time is up */
               //print_progress(0);
               m_internal_queue_size = m_mm_events_local.size();
            }
            break;      /* break this switch, but re-do the while loop.   */
      }//switch


      //TODO:wait for action mutex , m_action_cond ?
   }
   return 0;
}


void CRootWriter::clear_local_events()
{
   FreeClear(m_mm_events_local);
//	while (!m_mm_events_local.empty()) {
//		CMMEvent* mme = m_mm_events_local.front();
//		m_mm_events_local.erase(m_mm_events_local.begin());
//		delete mme; mme = 0;
//	}
}

const CEvent* const CRootWriter::current_event() const 
{
	//m_processed_data_mutex->lock(thread_id() , thread_name() , "current_event() ,1");
	if (m_mm_events_local.empty()) {
		//cout << "CRootWriter::m_mm_events_local.empty()" << endl;
		return 0;
	}
	CEvent* ev = m_mm_events_local.front();
	//cout << "m_mm_events_local.size()=" << m_mm_events_local.size() << " ev="<< ev << endl;
	//m_processed_data_mutex->unlock();
	return ev;
}


void CRootWriter::process_physics_event(const CMMEvent* const curr_mmevent)
{
	long nbytes = root_fill_tree_physics(curr_mmevent);
	m_bytes_written += nbytes;
}


void CRootWriter::process_pedestal_event(const CMMEvent* const  curr_mmevent)
{
   //local to change are mutable
   //new data from mmevent are const

   if (!curr_mmevent || curr_mmevent->apv_events().empty()) {
      return;
   }
   const std::list< CEvent* > curr_list_apv_events = curr_mmevent->apv_events();
	for (std::list< CEvent* > :: const_iterator evt = curr_list_apv_events.begin(); evt != curr_list_apv_events.end(); ++evt) {
		if ( (*evt)->is_bad() ) {
			//cerr << "process_pedestal_event(): is bad: apv_no=" <<  (*evt)->apv_no() << " event_number=" << (*evt)->event_number() << endl;
			return; //will not process - bad apvevent -> ignore bad mmevent 
		}
	}
	
   //if first run in series of m_pedestal_counts_per_save runs then just copy data into m_pedestals_mmevent
   if (!m_event_counter_pedestals) {
		m_event_counter_pedestals_done = 0L;
      m_pedestals_mmevent = *curr_mmevent;
      //m_pedestals_mmevent = *curr_mmevent;
      ++m_event_counter_pedestals;
      return; //?
   }

   //std::list< CApvEvent* > ped_list_apv_events = m_pedestals_mmevent.apv_events();
	
	for (std::list< CEvent* > :: const_iterator ievt = curr_list_apv_events.begin(); ievt != curr_list_apv_events.end(); ++ievt) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*> (*ievt);
		if (!apvevt) {
			continue;
		}
		if (!m_pedestals_mmevent.get_apv_event(apvevt->apv_id())) {
			cerr << "process_pedestal_event(): no such apv_id: apv_id=" <<  apvevt->apv_id() << endl;
			return; // no such apvid in writer's mmevent - ignore bad mmevent
		}	
	}
	
   //for each apvevent (128 channels) in curr_mmevent get the pedestal mean and std;
   for (std::list< CEvent* > :: const_iterator curr_event = curr_list_apv_events.begin(); curr_event != curr_list_apv_events.end(); ++curr_event) {
		CApvEvent* curr_apvevent = dynamic_cast<CApvEvent*> (*curr_event);
		if (!curr_apvevent) {
			continue;
		}
		//get apvevent for the same chip_id from the m_pedestals_mmevent
      CApvEvent* ped_apvevent = m_pedestals_mmevent.get_apv_event(curr_apvevent->apv_id());
      if (ped_apvevent == NULL) {
			cerr << "process_pedestal_event(): ped_apvevent == NULL - not found, go to next" <<endl;
         continue; // not found, go to next. TODO: error handling when no such apv_id
      }
		
		const std::map<int, std::vector<double> > current_event_pedestals = curr_apvevent->pedestal_data();
		std::map<int, std::vector<double> >& pedestal_map = 	ped_apvevent->pedestal_data_mutable();
		
		for (std::map<int, std::vector<double> > :: const_iterator icurr_ch = current_event_pedestals.begin(); icurr_ch != current_event_pedestals.end(); ++icurr_ch) {
			if (icurr_ch->second.size() != APV_PEDESTAL_VECTOR_SIZE) {
				cerr << "CRootWriter::process_pedestal_event(): pedestal vector bad size , channel ignored - todo: handle properly"<< endl;
				continue;
			}
			
			std::map<int, std::vector<double> > :: iterator ped_ch = pedestal_map.find(icurr_ch->first);
			if (ped_ch == pedestal_map.end()) {
				//chId not found
				cerr << "process_pedestal_event(): chId not found by pedestal.find(icurr_ch->first) , go to next" <<endl;
				continue;
			}
			
			//calculate for channel icurr_ch->first
			double curr_mean  = icurr_ch->second[0];
			double curr_stdev = icurr_ch->second[1];
			//double curr_sigma = icurr_ch->second[2];
		
			double prevmean  = pedestal_map[icurr_ch->first][0];
			double prevstdev = pedestal_map[icurr_ch->first][1];
			
			pedestal_map[icurr_ch->first][0] += (curr_mean - prevmean) / (m_event_counter_pedestals + 1); //mean of mean
         pedestal_map[icurr_ch->first][1] += (curr_stdev - prevstdev) / (m_event_counter_pedestals + 1); //mean of stdev
			pedestal_map[icurr_ch->first][2] += ( (curr_mean - prevmean) * (curr_mean - pedestal_map[icurr_ch->first][0] ) ); //sigma
			
		} //for std::map<int, std::vector<double> > :: const_iterator icurr_ch
		
   }//for std::list< CApvEvent* > :: const_iterator curr_apvevent

	//m_pedestals_mmevent.print();
	
	//check for overflow in an infinite loop
	bool force_save = false;
	if (std::numeric_limits<unsigned long>::max() - 1 > m_event_counter_pedestals) {
		++m_event_counter_pedestals;
		m_event_counter_pedestals_done = m_event_counter_pedestals;
	}
	else {
		force_save = true;
	}

	//this will execute when limit!=0 -> and automatic pedestal measurements are enabled (not implemented yet)
   if (force_save || (m_pedestal_counts_per_save && m_event_counter_pedestals > m_pedestal_counts_per_save)) {
		m_event_counter_pedestals_done = m_event_counter_pedestals;
      m_event_counter_pedestals = 0;
      long nbytes = root_fill_tree_pedestals(&m_pedestals_mmevent);
		m_bytes_written += nbytes;
      m_pedestals_mmevent.clear();
   }
}



int CRootWriter::root_open_file(std::string filename)
{
   if (!m_save_data_flag) {
      return 0;
   }
   CThreadRoot::lock_mutex();
   m_rootfile = new TFile(filename.c_str(), "RECREATE");
   if (!m_rootfile->IsOpen() || !m_rootfile->IsWritable()) {
      cerr << "Error: RawWriter::root_open_file(): Can't open " << filename << " for writing!" << endl;
      m_error = -1;
      CThreadRoot::unlock_mutex();
      return -1;
   }
	m_rootfile->cd();
   m_rawtree = new TTree("raw", "rawapvdata");
   if (!m_rawtree) {
      cerr << "Error: RawWriter::root_open_file(): creating raw tree." << endl;
      m_error  = -1;
      CThreadRoot::unlock_mutex();
      return -1;
   }
   if (!m_error) AddApvRawBranches(m_rawtree);
	
	m_rootfile->cd();
   m_pedtree = new TTree("pedestals", "apvpedestals");
   if (!m_pedtree) {
      cerr << "Error: RawWriter::root_open_file(): creating pedestals tree." << endl;
      m_error  = -1;
      CThreadRoot::unlock_mutex();
      return -1;
   }
   if (!m_error) AddApvPedestalsBranches(m_pedtree);

   //TODO: data tree
	m_rootfile->cd();
   m_datatree = new TTree("data", "apvdata");
   if (!m_datatree) {
      cerr << "Error: RawWriter::root_open_file(): creating data tree." << endl;
      m_error = -1;
      CThreadRoot::unlock_mutex();
      return -1;
   }
   if (!m_error) AddApvDataBranches(m_datatree);
	
	//TODO: data tree
	m_rootfile->cd();
   m_infotree = new TTree("run_info", "run_info");
   if (!m_infotree) {
      cerr << "Error: RawWriter::root_open_file(): creating m_infotree tree." << endl;
      m_error = -1;
      CThreadRoot::unlock_mutex();
      return -1;
   }
   if (!m_error) AddRunInfoBranches(m_infotree);
	
		
	
   CThreadRoot::unlock_mutex();
   //cout << "root_open_file() OK m_error = " << m_error << endl;
   return 0;
}


void CRootWriter::AddApvRawBranches(TTree* atree)
{
   //CConfiguration::runtype_t run_type = m_config->run_type();

   //Info("CRootFileWriter", "created root file %s and tree 0x%x\n", fname.Data(), m_rawtree);
   //create branches in tree: branches common to data and pedestal runs, then specific

   int ib = 0;
   m_rawbranches[ib++] = atree->Branch("apv_evt",   &m_apv_evt);
   m_rawbranches[ib++] = atree->Branch("time_s",    &m_time_s);
   m_rawbranches[ib++] = atree->Branch("time_us",   &m_time_us);

   m_rawbranches[ib++] = atree->Branch("apv_fecNo", &m_apv_fec);
   m_rawbranches[ib++] = atree->Branch("apv_id",    &m_apv_id);
   m_rawbranches[ib++] = atree->Branch("apv_ch",    &m_apv_ch);
   m_rawbranches[ib++] = atree->Branch("mm_id",     &m_mm_id);
   m_rawbranches[ib++] = atree->Branch("mm_readout", &m_mm_readout);
   m_rawbranches[ib++] = atree->Branch("mm_strip",  &m_mm_strip);

   m_rawbranches[ib++] = atree->Branch("apv_q",     &(m_apv_q));
   m_rawbranches[ib++] = atree->Branch("apv_presamples",     &m_apv_presamples);
	m_number_of_raw_tree_branches = ib;
   //8 branches


   for (int i = 0; i < m_number_of_raw_tree_branches; i++) {
      if (m_rawbranches[i] == 0) {
         cerr << "Error: AddApvRawBranches(): branch " << i << " is NULL" << endl;
         m_error = -2;
      }
   }
}


void CRootWriter::AddApvPedestalsBranches(TTree* ptree)
{
   //CConfiguration::runtype_t run_type = m_config->run_type();

   //Info("CRootFileWriter", "created root file %s and tree 0x%x\n", fname.Data(), m_rawtree);
   //create branches in tree: branches common to data and pedestal runs, then specific
   int ib = 0;
   m_pedbranches[ib++] = ptree->Branch("apv_evt",   &m_apv_evt);
   m_pedbranches[ib++] = ptree->Branch("time_s",    &m_time_s);
   m_pedbranches[ib++] = ptree->Branch("time_us",   &m_time_us);

   m_pedbranches[ib++] = ptree->Branch("apv_fecNo", &m_apv_fec);
   m_pedbranches[ib++] = ptree->Branch("apv_id",    &m_apv_id);
   m_pedbranches[ib++] = ptree->Branch("apv_ch",    &m_apv_ch);
   m_pedbranches[ib++] = ptree->Branch("mm_id",     &m_mm_id);
   m_pedbranches[ib++] = ptree->Branch("mm_readout", &m_mm_readout);
   m_pedbranches[ib++] = ptree->Branch("mm_strip",  &m_mm_strip);

   m_pedbranches[ib++] = ptree->Branch("apv_pedmean",  &(m_apv_pedmean));
   m_pedbranches[ib++] = ptree->Branch("apv_pedsigma", &(m_apv_pedsigma));
   m_pedbranches[ib++] = ptree->Branch("apv_pedstd",   &(m_apv_pedstd));
   m_number_of_ped_tree_branches = ib;
   //11 branches

   for (int i = 0; i < m_number_of_ped_tree_branches; i++) {
      if (m_pedbranches[i] == 0) {
         cerr << "Error: AddApvPedestalsBranches(): branch " << i << " is NULL" << endl;
         m_error = -3;
      }
   }
}


void CRootWriter::AddApvDataBranches(TTree* datatree)
{
   int ib = 0;
   m_databranches[ib++] = datatree->Branch("apv_qmax",  &(m_apv_qmax));
   m_databranches[ib++] = datatree->Branch("apv_tbqmax", &(m_apv_tbqmax));
   m_number_of_data_tree_branches = ib;
   for (int i = 0; i < m_number_of_data_tree_branches; i++) {
      if (m_databranches[i] == 0) {
         cerr << "Error: AddApvDataBranches(): branch " << i << " is NULL" << endl;
         m_error = -4;
      }
   }
}

void CRootWriter::AddRunInfoBranches(TTree* datatree)
{
   int ib = 0;
   m_infobranches[ib++] = datatree->Branch("comment", &(m_info_comment));
   m_infobranches[ib++] = datatree->Branch("zero_factor",  &(m_info_zero_factor));
   m_number_of_data_tree_branches = ib;
   for (int i = 0; i < m_number_of_data_tree_branches; i++) {
      if (m_infobranches[i] == 0) {
         cerr << "Error: AddRunInfoBranches(): branch " << i << " is NULL" << endl;
         m_error = -5;
      }
   }
}

int CRootWriter::root_fill_tree_runinfo()
{
	assert(m_root_thread);
   if (m_error || !m_save_data_flag || !m_root_thread) {
      return 0;
   }

	m_info_comment = m_config ->comments();
	CThreadRoot::lock_mutex();
   int ib = 0; // warning: keep the same sequence as in add_branches()
	std::string* pstr = &m_info_comment;
	m_infobranches[ib++]->SetAddress(&pstr);
   m_infobranches[ib++]->SetAddress(&m_info_zero_factor);
	CThreadRoot::unlock_mutex();
	
   std::cout << std::endl;
   std::cout << "CRootWriter::root_fill_tree_runinfo() : m_info_comment" << m_info_comment << std::endl;
   std::cout << "CRootWriter::root_fill_tree_runinfo() : m_info_zero_factor" << m_info_zero_factor << std::endl;
   
	//   CThreadRoot::lock_mutex();
   m_infotree->SetDirectory(m_rootfile);
   int rc = m_infotree->Fill();
      std::cout << "CRootWriter::root_fill_tree_runinfo() :fill rc=" << rc << std::endl;
   if (rc == -1) {
      cerr << "ERROR: root_fill_tree_runinfo(): Fill run_info tree error" << endl;
		return 0;
   }
  	//  CThreadRoot::unlock_mutex();
   return rc;

	
	
}


int CRootWriter::root_fill_tree_physics(const CMMEvent* mm_event)
{
	//TODO: fill int presamples_count() from CAPVEvent
	assert(m_root_thread);
   if (m_error || !m_save_data_flag || !m_root_thread) {
      return 0;
   }

   //CConfiguration::runtype_t run_type = m_config->run_type();

   // bool eventHasData = false;
   //cout << "root_fill_tree(CMMEvent* mm_event): clear()" << endl;
   m_apv_fec.clear();
   m_apv_ch.clear();
   m_apv_id.clear();
   m_mm_id.clear();
   m_mm_readout.clear();
   m_mm_strip.clear();
   m_apv_q.clear();
	m_apv_presamples= 0;
   m_apv_qmax.clear();
   m_apv_tbqmax.clear();

   std::vector <UInt_t>*			papv_fec     = &m_apv_fec;
   std::vector <UInt_t>*			papv_id      = &m_apv_id;			//APVChip;
   std::vector <UInt_t>*			papv_ch      = &m_apv_ch;			//APVChannel;
   std::vector <std::string>*		pmm_id		 = &m_mm_id;
   std::vector <UInt_t>*			pmm_readout  = &m_mm_readout;
   std::vector <UInt_t>*			pmm_strip    = &m_mm_strip;
   std::vector <  std::vector<Short_t> >* papv_q      = &m_apv_q;	// time bns q
   std::vector <Short_t>*			papv_qmax   = &m_apv_qmax;
   std::vector <Short_t>*			papv_tbqmax = &m_apv_tbqmax;


   //const std::map<unsigned, int > channel_map  = m_config->channel_map() ;

   //cout << "root_fill_tree(CMMEvent* mm_event): loop" << endl;
   //has std::list< CApvEvent* > m_apv_events;

   //	CEvent::event_type_type event_type = mm_event->event_type();
   m_apv_evt = mm_event->event_number();
   m_time_s = (int)mm_event->event_time_sec();
   m_time_us = mm_event->event_time_usec();
	m_apv_presamples = mm_event->presamples_count();
	
   const std::list< CEvent* > apv_events = mm_event->apv_events();
   for (std::list< CEvent* > :: const_iterator ievent = apv_events.begin() ; ievent != apv_events.end(); ++ievent) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*> (*ievent);
		if (!apvevt) {
			continue;
		}
		
      const std::map<int, std::vector<int16_t> >* const event_data = apvevt->apv2d_data();
      for (std::map<int, std::vector<int16_t> > :: const_iterator istrip = event_data->begin(); istrip != event_data->end(); ++istrip) {
			
			if ( (m_config->strip_from_channel(istrip->first) - 1) < 0)
            continue; // check for unmapped channels ? unnecessary??
			
         string name = m_config->chamber_name_by_chip(apvevt->apv_id()) ;
         const CDetChamber* chamber = m_config->chamber_by_name(name);
         if (!chamber) {
            continue;
         }
         const CDetAbstractReadout* readout = chamber->get_readout_for_chipId(apvevt->apv_id());
         
         int readoutid = readout ? readout->get_id() : 0;
         UInt_t mmstrip = (m_config->strip_from_channel(istrip->first) - 1);
         
	//hack for MBT0 chambers in ATLAS (mixed XV strips on the two APV, V strips are mapped +200)
		if(name.substr(0,4).compare("MBT0") == 0) {
			if(mmstrip > 200) {
				mmstrip -= 200;
				readoutid = 0; // sorting readouts alphabetically gives: V=0, X=1
			} else {
			readoutid = 1;
			} 
		}

         m_apv_fec.push_back(apvevt->fec_no());
         m_apv_id.push_back(apvevt->apv_no());
         m_mm_id.push_back(name);						//TODO: mapping geo
         m_mm_readout.push_back( readoutid );
         int apv_chNo = CApvEvent::chanNo_from_chId(istrip->first); //istrip->first & 0xFF;
         m_apv_ch.push_back(apv_chNo);
         m_mm_strip.push_back(mmstrip) ; // error chec
         m_apv_q.push_back(istrip->second);
			
			
         //max values
         std::vector<int16_t>::const_iterator imax = max_element(istrip->second.begin(), istrip->second.end());
         int16_t valqmax = *imax;
         int16_t tbqmax  = imax - istrip->second.begin() ; // time bin counting from 0 : removed "+ 1" (agrees with fitting)
         m_apv_qmax.push_back(valqmax);
         m_apv_tbqmax.push_back(tbqmax);

      } // for istrip

   }//for ievent

   //m_event_type = eventTypePhysics;

   //cout << "root_fill_tree(CMMEvent* mm_event): SetAddress()" << endl;
   //fill root tree with data (do not fill with pedestals)
   CThreadRoot::lock_mutex();
   int ib = 0; // warning: keep the same sequence as in add_apv_branches()
   m_rawbranches[ib++]->SetAddress(&m_apv_evt);
   m_rawbranches[ib++]->SetAddress(&m_time_s);
   m_rawbranches[ib++]->SetAddress(&m_time_us);

   m_rawbranches[ib++]->SetAddress(&papv_fec);
   m_rawbranches[ib++]->SetAddress(&papv_id);
   m_rawbranches[ib++]->SetAddress(&papv_ch);
   m_rawbranches[ib++]->SetAddress(&pmm_id);
   m_rawbranches[ib++]->SetAddress(&pmm_readout);
   m_rawbranches[ib++]->SetAddress(&pmm_strip);

   m_rawbranches[ib++]->SetAddress(&papv_q);
   m_rawbranches[ib++]->SetAddress(&m_apv_presamples);
	int id_data = 0;
   m_databranches[id_data++]->SetAddress(&papv_qmax);
   m_databranches[id_data++]->SetAddress(&papv_tbqmax);
   CThreadRoot::unlock_mutex();

	
	
//   CThreadRoot::lock_mutex();
   m_rawtree->SetDirectory(m_rootfile);
   int rc1 = m_rawtree->Fill();
   if (rc1 == -1) {
      cerr << "ERROR: root_fill_tree_physics(): Fill raw tree error" << endl;
		return 0;
   }
   m_datatree->SetDirectory(m_rootfile);
   int rc2 = m_datatree->Fill();
   if (rc2 == -1) {
      cerr << "ERROR: root_fill_tree_physics(): Fill data tree error" << endl;
		return 0;
   }
 //  CThreadRoot::unlock_mutex();
   return rc1 + rc2;
}


int CRootWriter::root_fill_tree_pedestals(const CMMEvent* mm_event)
{
	assert(m_root_thread);
   if (m_error || !m_save_data_flag || !m_root_thread) {
      return 0;
   }
//   cout << "CRootWriter::root_fill_tree_pedestals() "<< endl;

   //CConfiguration::runtype_t run_type = m_config->run_type();

   // bool eventHasData = false;
   m_apv_fec.clear();
   m_apv_ch.clear();
   m_apv_id.clear();
   m_mm_id.clear();
   m_mm_readout.clear();
   m_mm_strip.clear();

   m_apv_pedmean.clear();
   m_apv_pedsigma.clear();
   m_apv_pedstd.clear();

   std::vector <UInt_t>*			papv_fec     = &m_apv_fec;
   std::vector <UInt_t>*			papv_id      = &m_apv_id;			//APVChip;
   std::vector <UInt_t>*			papv_ch      = &m_apv_ch;			//APVChannel;
   std::vector <std::string>*		pmm_id		 = &m_mm_id;
   std::vector <UInt_t>*			pmm_readout  = &m_mm_readout;
   std::vector <UInt_t>*			pmm_strip    = &m_mm_strip;

   std::vector <float>*				papv_pedmean  = &m_apv_pedmean;
   std::vector <float>*				papv_pedsigma = &m_apv_pedsigma;
   std::vector <float>*				papv_pedstd   = &m_apv_pedstd;

   //TODO: push_back() loop here

   //const std::map<unsigned, int > channel_map  = m_config->channel_map() ;
//   cout << "root_fill_tree(CMMEvent* mm_event): loop, mm_event->apv_events() size = " << mm_event->apv_events().size() << endl;
   //has std::list< CApvEvent* > m_apv_events;


   m_apv_evt = mm_event->event_number();
   m_time_s  = (int)mm_event->event_time_sec();
   m_time_us = mm_event->event_time_usec();
   const std::list< CEvent* > apv_events = mm_event->apv_events();

   //std::vector<double> dev_of_mean;
   for (std::list< CEvent* > :: const_iterator ievent = apv_events.begin() ; ievent != apv_events.end(); ++ievent) {
		CApvEvent* apvevt = dynamic_cast<CApvEvent*> (*ievent);
		if (!apvevt) {
			continue;
		}
//      cout << "___________________________________________________________________________________________________________" << endl;
//		(*ievent)->print();
//      cout << "___________________________________________________________________________________________________________" << endl;
		
		const std::map<int, std::vector<double> > pedestals = apvevt->pedestal_data();
		for (std::map<int, std::vector<double> > :: const_iterator ichan = pedestals.begin(); ichan != pedestals.end(); ++ichan) {
			int apvIdCh = ichan->first;
			int chNo = CApvEvent::chanNo_from_chId(apvIdCh); // (apvIdCh & 0xFF);
			int mmstripNo = m_config->strip_from_channel(apvIdCh) - 1;
			if (mmstripNo < 0){
            continue;
			}
			double mean_of_mean  = ichan->second[0];
			double mean_of_stdev = ichan->second[1];
			double sigma_of_mean = ichan->second[2];
			
			string name = m_config->chamber_name_by_chip(apvevt->apv_id()) ;
         const CDetChamber* chamber = m_config->chamber_by_name(name);
//         if (!chamber) {
//            continue;
//         }
			if(!chamber) {
				std::cerr << " Bad config: Apv id not assigned to chamber " << apvevt->apv_id() << " -  not saved. "<< std::endl;
				continue;
			}
         const CDetAbstractReadout* readout = chamber->get_readout_for_chipId(apvevt->apv_id());
//         std::cout << "root_fill_tree_pedestals() : readout=" << readout << std::endl;
         

			m_apv_fec. push_back(apvevt->fec_no());
			m_apv_id . push_back(apvevt->apv_no());
         int readoutid = readout ? readout->get_id() : 0;

         // hack for MBT0 chambers in ATLAS (mixed XV strips on 2 APVs, V strips are +200)
		if(name.substr(0,4).compare("MBT0") == 0) {
			if(mmstripNo > 200) {
				mmstripNo -=200;
				readoutid = 0; // sorting readouts alphabetically gives: V=0, X=1
			} else {
			readoutid = 1;
			} 
		}
	 
			//TODO: mapping geo TODO: change to int id
			m_mm_id.      push_back(name);
         m_mm_readout. push_back( readoutid );
         m_apv_ch.     push_back(chNo);
         m_mm_strip.   push_back(mmstripNo);
         m_apv_pedmean.push_back(static_cast<Float_t>(mean_of_mean));
         m_apv_pedstd. push_back(static_cast<Float_t>(mean_of_stdev));
			// sigma see this http://www.johndcook.com/standard_deviation.html
			Float_t s = m_event_counter_pedestals_done > 1 ? sqrt(sigma_of_mean / (double)(m_event_counter_pedestals_done - 1) ) : 0.0 ;
         m_apv_pedsigma.push_back(s);
		}
		
   }//for ievent

	
	
   //CThreadRoot::lock_mutex();
   int ib_ped = 0;
   m_pedbranches[ib_ped++]->SetAddress(&m_apv_evt);
   m_pedbranches[ib_ped++]->SetAddress(&m_time_s);
   m_pedbranches[ib_ped++]->SetAddress(&m_time_us);

   m_pedbranches[ib_ped++]->SetAddress(&papv_fec);
   m_pedbranches[ib_ped++]->SetAddress(&papv_id);
   m_pedbranches[ib_ped++]->SetAddress(&papv_ch);
   m_pedbranches[ib_ped++]->SetAddress(&pmm_id);
   m_pedbranches[ib_ped++]->SetAddress(&pmm_readout);
   m_pedbranches[ib_ped++]->SetAddress(&pmm_strip);

   m_pedbranches[ib_ped++]->SetAddress(&papv_pedmean);
   m_pedbranches[ib_ped++]->SetAddress(&papv_pedsigma);
   m_pedbranches[ib_ped++]->SetAddress(&papv_pedstd);
   //cout << "root_fill_tree(CMMEvent* mm_event): m_rawtree Fill() -pedestals" << endl;

   m_pedtree->SetDirectory(m_rootfile);
   int rc = m_pedtree->Fill();
   if (rc == -1) {
      cerr << "ERROR: root_fill_tree_pedestals(): Fill pedestals tree error" << endl;
		return 0;
   }
   //cout << "Fill() rc=" << rc << endl;
   //CThreadRoot::unlock_mutex();
   return rc;
}



int CRootWriter::root_write_file()
{
   // write info for run , not for events
   //erase from tree (already filled  per event) ? then write?
   if (!m_save_data_flag) {
      return 0;
   }
   if (!m_rootfile) {
      return 0;
   }

   //special case for pedestals
   //in other parts ot the program this is defined as event attribute - eventType_t
   if (m_config->run_type() ==  CConfiguration::runtypePedestals && !m_pedestal_counts_per_save) {
      long nbytes = root_fill_tree_pedestals(&m_pedestals_mmevent);
		m_bytes_written += nbytes;
   }
	long nbytes = root_fill_tree_runinfo();
	m_bytes_written += nbytes;
	

   cout << "root_write_file(): m_rootfile->Write()" << endl;

   CThreadRoot::lock_mutex();
   m_rootfile->Write();
   m_rootfile->cd();
   m_rootfile->mkdir("config", "parameters from config file");
   m_rootfile->Cd("config");
   m_config->get_tenv_config()->Write();
   CThreadRoot::unlock_mutex();
   return 0;
}


int CRootWriter::root_close_file()
{
   //cout << "root_close_file(): m_rawtree->Close()" << endl;

   CThreadRoot::lock_mutex();
   m_rootfile->Close();
   CThreadRoot::unlock_mutex();
   //   m_rootfile = 0;
   return 0;
}










