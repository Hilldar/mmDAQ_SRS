/*
 *  CConfiguration.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/27/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CConfiguration_h
#define CConfiguration_h

#include "MBtools.h"


#include <TString.h>

#include <vector>
#include <string>
#include <map>
#include <set>
#include <utility>

class CApvRawPedRoot;

class CDetFec;
class CDetChip;
class CDetChamber;
class CEvent;
class CMMEvent;

class TEnv;

// from http://root.cern.ch/root/html/tutorials/gui/buttongroupState.C.html




class CConfiguration {

public: 
	enum runtype_t { runTypeBad = 0, runtypePhysics = 1, runtypePedestals = 2 };

public:
	CConfiguration(int argc = 0, char **argv = 0, const char* filename = 0);
	~CConfiguration();
	
	void load(const char* new_config_file);
	void save(const char* new_config_file);
	
	void HandleArgs( int argc, char **argv);
	int  load_configuration();
	int  load_pedestals(const char* filename = 0);
	void clear_pedestals();
	
	const bool			ignoring()		const { return m_ignore_run;}
	const bool        suppressing_zeros() const { return m_suppresing_zeros;}
	const std::string comments()		const { return m_comments;}
	void comments( std::string str) { m_comments = str;}
	void comments( const char* str) { m_comments = std::string(str);}
	bool is_common_mode_enabled() const { return m_correct_common_mode; }
   void set_common_mode_enabled(bool state) { m_correct_common_mode = state;}
   void set_common_mode_factor(double val) { m_correct_common_mode_factor = val;}
   double get_common_mode_factor() const { return m_correct_common_mode_factor;}
   
	//geo
	size_t							chip_count()	const { return m_number_of_chips;}
	std::vector<std::string>	chip_list_names() const;
	const std::vector<CDetChip*>&		chip_list() const { return m_defined_chips;}

	const std::vector<CDetFec*>&		defined_fecs() const     { return m_defined_fecs;}
	const std::vector<CDetChip*>&		defined_chips() const    { return m_defined_chips;}
	const std::vector<CDetChamber*>&	defined_chambers() const { return m_defined_chambers;}
	
	std::vector<std::string >  defined_fec_names() const ;
	std::vector<std::string >  defined_fec_ip_addresses() const ;
	std::vector<int>				defined_chip_ids() const ;
	std::vector<std::string >  defined_chip_names() const ;
	std::vector<std::string >  defined_chamber_names() const ;
	std::string						defined_chamber_name(const int chamber_id) const ;
	std::pair<unsigned,unsigned> defined_chamber_strip_ranges(int chamber_id) const;
	std::map< int, std::pair<unsigned,unsigned> > defined_chamber_strip_ranges() const { return m_defined_chamber_strip_ranges;};
	
	const std::multimap <int,int>& lookup_chip_chamber() const { return m_lookup_chip_chamber;}; 
	const std::multimap <int,int>& lookup_chamber_chip() const { return m_lookup_chamber_chip;};
	
	const std::vector< CDetChip*> chips_on_chamber(const int chamber_id);
	const CDetChamber* const	chamber_by_name(const std::string name) const;
	const int						chamber_id_by_chip(const int chip_id) const;
	const std::string				chamber_name_by_chip(const int chip_id) const;
	const CDetChamber* const	get_chamber_by_chipId(int chipId)  const;
	
   const TEnv* get_tenv_config() const { return m_env;}
   


   const std::vector< std::pair<std::string, double> >  get_chamber_readouts(const std::string& chamber_name) const {
      std::map<std::string, std::vector< std::pair<std::string, double> > > :: const_iterator found = m_lookup_chamber_readout_angle.find(chamber_name);
      if (found != m_lookup_chamber_readout_angle.end()) {
         return found->second;;
      }
      return std::vector< std::pair<std::string, double> >();
   }
   
   double get_chamber_readout_angle(const std::string& chamber_name, const std::string& readoutname ) const {
      std::vector< std::pair<std::string, double> > readouts = get_chamber_readouts(chamber_name);
      for (std::vector< std::pair<std::string, double> >::const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
         if (ird->first == readoutname) {
            return ird->second;
         }
      }
      std::cerr << "CConfiguration::get_chamber_readout_angle() : no such chamber or readout" << std::endl;
      throw std::string("CConfiguration::get_chamber_readout_angle() : no such chamber or readout");
      //return 0.0;
   }
   
	
	//other
	const runtype_t	run_type()		const { return m_type_of_run;}
	void					run_type(runtype_t type) { m_type_of_run = type;};
	int					load_log();
	int					get_next_run_number_from_log();
	const char*			log_filename() const	{ return m_log_file.c_str();}
	const char*       pedestal_filename() const { return m_pedestal_file.c_str();}
	void					pedestal_filename(const char* name);
	const int			run_number()	const { return m_run_number;}
	const int			error()			const { return m_error; }
	const CMMEvent* const pedestal_event() const { return m_pedestals_event; }
   bool pedestal_chip_mean(unsigned chipId, double& pedestal) const;
   std::vector<double> const& pedestal_chip_mean_vector(int chipId);
   std::vector<double> const& pedestal_chip_stdev_vector(int chipId);
	const bool			pedestal_mean(const unsigned chId, double& pedestal) const;
	const bool			pedestal_stdev(const unsigned chId, double& value) const;
	const double      sigma_cut() const { return m_sigma_cut_factor;}
	void					sigma_cut(double val) { m_sigma_cut_factor = val;}
	const int         strip_from_channel(const int apvIdCh) const;
	const int			strip_from_channel(const int apv_id, const int chNo) const;
   int channel_from_strip(const int stripId) const;
	const char*       daq_ip_address() const { return m_daq_ip_address.c_str();}
	const char*       daq_ip_port() const { return m_daq_ip_port.c_str();}
	const char*       config_file() const { return m_config_file.c_str();}	
	//const std::vector<std::string>& fec_ip_address() const { return m_fec_ip_address;}
	const std::string	write_path() const { return m_write_data_path;}
	
	const std::map<int, double>* pedestal_mean_map() const { return &m_pedestals_mean;}
	const std::map<int, double>* pedestal_stdev_map() const { return &m_pedestals_stdev;}
	const std::map<unsigned, int >* channel_map() const { return &m_channel_map;}
	const int							pedestal_events_per_save() const { return m_pedestal_events_per_save;}
	const std::map<unsigned, std::vector <unsigned> >& crosstalk_map() const { return m_crosstalk_map;} 

   //geo helpers	
	CDetChip* find_defined_chip(const std::string name) ;
	CDetChip* find_defined_chip(const int chip_id);
	const CDetChip* const find_defined_chip(const std::string name) const ;
	const CDetChip* const find_defined_chip(const int chip_id) const;
	CDetChamber* find_defined_chamber(const int chamb_id);
	const CDetChamber* const find_defined_chamber(const std::string name) const ;
	const CDetChamber* const find_defined_chamber(const int chamb_id) const;
	
private:
	IDList IDs;

	int m_error;
	std::string m_config_file;
	std::string m_log_file;
	std::string m_pedestal_file;
	std::string m_pedestal_path;
	std::string m_write_data_path;
	std::string m_map_file;
	std::string m_crosstalk_file;
	std::string m_daq_ip_address;
	std::string m_daq_ip_port;
	std::vector<std::string> m_fec_ip_address;

   bool m_correct_common_mode;
	bool			m_suppresing_zeros;
	bool			m_ignore_run;
	runtype_t	m_type_of_run;
	double m_correct_common_mode_factor;
	
	int			m_run_number;
	std::string m_comments;
	size_t		m_number_of_chips;
	
	// info from the config file
	//geometry
	std::vector<CDetFec*>		m_defined_fecs;		//list of FEC objects
	std::vector<CDetChip*>		m_defined_chips;		//declared chips
	std::vector<CDetChamber*>	m_defined_chambers;	//declared chambers
	
	std::map <int, std::pair<unsigned,unsigned> > m_defined_chamber_strip_ranges; //chamber_id -> stipLO, stripHI

	
	std::map <int,int>		m_lookup_chip_fec;		// these 2 we do not need
	std::map <int,int>		m_lookup_fec_chip;

	std::multimap <int,int>				m_lookup_chip_chamber; //lookup tables id by id
	std::multimap <int,int>				m_lookup_chamber_chip;
	std::multimap <std::string,int>	m_lookup_chamber_name_chip; //lookup tables id by id
	std::multimap <int,std::string>	m_lookup_chip_chamber_name;	
	
   std::map<std::string, std::vector< std::pair<std::string, double> > > m_lookup_chamber_readout_angle;
	
	std::vector <std::string> m_log_entries;	//TODO: change string to RunInfo

//	CApvRawPedRoot*	     m_root_ped;
	CMMEvent*						m_pedestals_event;
	std::map<int, double>		m_pedestals_mean;
	std::map<int, double>		m_pedestals_stdev;
	double							m_sigma_cut_factor;
	int								m_pedestal_events_per_save;	// when auto measuring pedestals  how many events to measure before saving mean to disk
																				//	(TODO: implement) auto measurement of pedestals
   std::map<int, std::vector<double> > m_pedestal_chip_mean_lookup;
   std::map<int, std::vector<double> > m_pedestal_chip_stdev_lookup;
   
	std::map<unsigned, int >	m_channel_map;
	std::map<unsigned, int >	m_strip_map;
   
	std::map<unsigned, std::vector <unsigned> > m_crosstalk_map;

	//geometry
	std::map <unsigned, std::string> m_geo_ApvToChamber; //apvId(fec+chip) to mm chamberName;
   TEnv* m_env;

   
private:
	CConfiguration(CConfiguration&);
	CConfiguration& operator=(CConfiguration&);
	void clear();
	
	
	void FillCSVToIntVector(TString &line, std::vector<int>& vec, const char* delim);
	void FillCSVToVector(TString &line, std::vector<std::string>& list, const char* delim = 0);
	void PrintConfig() const;
	int load_channel_map();
	int load_crosstalk_map();
	bool locate_pedestal_file(std::string& filename);

	
	TString& remove_quotes(TString& item) { item.ReplaceAll("\"", ""); return item;};
	const void usage(const char *ourName) const;


};

#endif
