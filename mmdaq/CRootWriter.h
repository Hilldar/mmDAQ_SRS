/*
 *  CRootWriter.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 3/9/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */


#ifndef CRootWriter_h
#define CRootWriter_h

#include "CThread.h"
#include "CThreadRoot.h"
#include "CMutex.h"
#include "CMMEvent.h"
#include "CMMDaq.h"
#include "CUserInterface.h"
#include <TROOT.h>


//#include <utility>
#include <list>
#include <map>
#include <vector>
#include <list>
#include <string>


class CConfiguration;
class CEvent;
class CMMEvent;

class TFile;
class TTree;
class TBranch;
class TCondition;

class CRootWriter : public CThread, public CThreadRoot {
	
public:
//	CRootWriter(CMMDaq* daq, CConfiguration* config,
//              std::list <CMMEvent*>* eventcontainer,
//              pthread_mutex_t& action_mutex,
//              pthread_mutex_t& data_mutex,
//              pthread_cond_t& cond,
//				  TCondition* cond_proc,
//				  bool save_data);
	CRootWriter(CMMDaq* daq, CConfiguration* config,
					std::list <CMMEvent*>* eventcontainer,
					CMutex* published_mutex,
					CMutex* processed_mutex,
					pthread_cond_t& cond,
					TCondition* cond_proc,
					bool save_data);
	
   ~CRootWriter();
	
	void	set_save_data_flag(bool state) { m_save_data_flag = state;} 
   int	main(void*);
	int	root_write_file();
   int	root_close_file();
	const long bytes_written() const { return m_bytes_written;}
	
   //derived from CThread
   virtual int execute_root_thread() {
      return main((void*)0);
   };
   virtual int execute_thread() {
      return main((void*)0);
   };
	
	int run_number() {
      return m_runnumber;
   };
   int event_count() {
      return m_event_counter;
   }
	
	const CEvent* const current_event() const ;
	
private:
   //writer members

   CMMDaq* 						m_daq;	
   int							m_error;
   CConfiguration*			m_config;
   std::list <CMMEvent*>*	m_mm_events_decoded;
   std::list <CMMEvent*>	m_mm_events_local;
	CMMEvent						m_pedestals_mmevent;
	long							m_bytes_written;
	//m_statistics_mmevent
	CMutex*						m_publishing_mutex;
	CMutex*						m_processed_data_mutex;
	
//   pthread_mutex_t*			m_publishing_mutex;
//   pthread_mutex_t*			m_processed_data_mutex;

   pthread_cond_t*			m_action_event_processed;
	TCondition*					m_condition_processed;


	
   TFile*						m_rootfile;
   TTree*						m_rawtree;
	TTree*						m_pedtree;
	TTree*						m_datatree;
	TTree*						m_infotree;
   std::vector<TBranch*>	m_rawbranches;
	std::vector<TBranch*>	m_pedbranches;
	std::vector<TBranch*>	m_databranches;
	std::vector<TBranch*>	m_infobranches;
	
   int				m_event_counter;
	unsigned long	m_event_counter_pedestals;
	unsigned long	m_event_counter_pedestals_done;
	unsigned long	m_pedestal_counts_per_save; //TODO: to be defined by user - put to 0 to save at the end of pedestal run
   //event_type_type	m_event_type;
	bool				m_event_has_data;
	bool				m_save_data_flag;
   int				m_runnumber;
   int				m_number_of_raw_tree_branches;
	int				m_number_of_ped_tree_branches;
	int				m_number_of_data_tree_branches;
	
   //root - raw tree data - apv data to be written to root file - public?
   UInt_t								m_apv_evt;
	/*time_t	*/	int					m_time_s;
	/*suseconds_t*/int				m_time_us;
   std::vector <UInt_t>				m_apv_fec;
   std::vector <UInt_t>				m_apv_id;				//APVChip;
   std::vector <UInt_t>				m_apv_ch;				//APVChannel;
   std::vector <std::string>		m_mm_id;					//MM chamber id
   std::vector <UInt_t>				m_mm_readout;			//MM readout plane;
   
   std::vector <UInt_t>				m_mm_strip;				//MM strip id
   std::vector <std::vector <Short_t> >  m_apv_q;		// time bns q
	UInt_t				m_apv_presamples;		// number of presamples in apv data
   //root - raw tree data - for the pedestal root file
   std::vector <Float_t>			m_apv_pedmean;
   std::vector <Float_t>			m_apv_pedsigma;
   std::vector <Float_t>			m_apv_pedstd;
   //root - data tree data -  for the physics run root file
   std::vector <Short_t>			m_apv_qmax;
   std::vector <Short_t>			m_apv_tbqmax;
	//root - run info tree data
	std::string							m_info_comment;
	double								m_info_zero_factor;
   

	
   std::map<int, double>		m_pedestals_mean;  // mean  of mean_i
   std::map<int, double>		m_pedestals_sigma; // stdev of mean_i
   std::map<int, double>		m_pedestals_stdev; // mean  of stddev_i
   std::map<unsigned, int >	m_channel_map;
	
	
public:
	const size_t internal_queue_size() const { return m_internal_queue_size;}
	long internal_queue_decoded_size() const { return m_internal_queue_decoded;}
	size_t m_internal_queue_size;
	long m_internal_queue_decoded;
private:
	CRootWriter(const CRootWriter&);
	CRootWriter& operator=(const CRootWriter&);
	
	
	void clear_local_events();
   int root_open_file(std::string filename) ;
	int root_fill_tree_runinfo();
   int root_fill_tree_physics(const CMMEvent* event) ;
   int root_fill_tree_pedestals(const CMMEvent* event) ;

   void AddApvRawBranches(TTree* atree);
	void AddApvPedestalsBranches(TTree* ptree);
	void AddApvDataBranches(TTree* datatree);
	void AddRunInfoBranches(TTree* datatree);

	void process_pedestal_event(const CMMEvent* const event);
	void process_physics_event(const CMMEvent* const curr_mmevent);


	
	
};

#endif

