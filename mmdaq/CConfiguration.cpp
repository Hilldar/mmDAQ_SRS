/*
 *  CConfiguration.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/27/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CConfiguration.h"
#include "CApvRawPedRoot.h"
#include "CApvPedestalsRoot.h"
#include "CDetElement.h"
#include "CDetFec.h"
#include "CDetChip.h"
#include "CDetChamber.h"
#include "CDetReadout.h"
#include "CUserInterface.h"
#include "CEvent.h"
#include "CMMEvent.h"

#include "MBtools.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <sys/stat.h>

#include <boost/bind.hpp>

#include <TEnv.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TString.h>
#include <TSystem.h>

#define MMDAQ_LOG_FILENAME "mmdaq_test.log"
#define MMDAQ_LOG_PATH "."

#define MMDAQ_LOWEST_RUNNUMBER 1
#define MMDAQ_BAD_RUNNUMBER (-1)

#ifndef NUMBEROFAPVCHANNELS
#define NUMBEROFAPVCHANNELS 128
#endif

#ifndef MAXNUMBEROFAPVCHIPS
#define MAXNUMBEROFAPVCHIPS 16
#endif

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using std::set;
using std::pair;
using std::multimap;


CConfiguration::CConfiguration(int argc, char **argv, const char* filename)
      : IDs(), m_error(0), m_config_file((filename && strlen(filename)) ? filename : "") , 
		m_log_file(""), m_pedestal_file(""), m_pedestal_path(""), m_write_data_path(""), m_map_file(""), m_crosstalk_file(""),
      m_daq_ip_address(""), m_daq_ip_port(""),  m_fec_ip_address(), m_correct_common_mode(true), m_suppresing_zeros(false),
      m_ignore_run(false),  m_type_of_run(runtypePhysics), m_correct_common_mode_factor(10.0), m_run_number(MMDAQ_BAD_RUNNUMBER), m_comments(""),
      m_number_of_chips(0),
      m_defined_fecs(), m_defined_chips(), m_defined_chambers(), m_defined_chamber_strip_ranges(), m_lookup_chip_fec(), m_lookup_fec_chip(),
      m_lookup_chip_chamber(), m_lookup_chamber_chip(), m_lookup_chamber_name_chip(), m_lookup_chip_chamber_name(), 
m_lookup_chamber_readout_angle(),

      m_log_entries(), m_pedestals_event(new CMMEvent()), m_pedestals_mean(), m_pedestals_stdev(), m_sigma_cut_factor(0), m_pedestal_events_per_save(0),
      m_pedestal_chip_mean_lookup(), m_pedestal_chip_stdev_lookup(), m_channel_map(), m_strip_map(), m_crosstalk_map(), m_geo_ApvToChamber(), m_env(0)
{

   m_log_file = MMDAQ_LOG_FILENAME;

   HandleArgs(argc, argv);
   if (!m_error) m_error += load_configuration();

   if (!m_error) m_error += load_log();

   if (!m_error && !m_pedestal_file.empty()) {
      m_error += load_pedestals();
   }

   if (!m_error) { m_error += load_channel_map(); }
	if (!m_error) { m_error += load_crosstalk_map(); }
   if (!m_error) m_run_number = get_next_run_number_from_log();

   if (m_error) {
      throw string("Error: CConfiguration: initialisations ");
   }

   PrintConfig();

}


CConfiguration::~CConfiguration()
{
	delete m_env;
	delete m_pedestals_event; m_pedestals_event = 0;
	FreeClear(m_defined_fecs);	
	FreeClear(m_defined_chips);	
	FreeClear(m_defined_chambers);
	
};	


void CConfiguration::load(const char* new_config_file)
{
	if (!new_config_file ) {
		return;
	}
	clear();
	m_config_file = new_config_file;
	if (!m_error) m_error += load_configuration();
   if (!m_error) m_error += load_log();
	if (!m_error && !m_pedestal_file.empty()) {
      m_error += load_pedestals();
   }
	if (!m_error) m_error += load_channel_map();
	if (!m_error) { m_error += load_crosstalk_map(); }
   if (!m_error) m_run_number = get_next_run_number_from_log();

	PrintConfig();
}

void CConfiguration::save(const char* new_config_file)
{
	TEnv env;
	
	env.SetValue("mmdaq.DAQIPAddress",	"127.0.0.1",kEnvLocal);
   env.SetValue("mmdaq.DAQIPPort",		"6006",kEnvLocal);
   env.SetValue("mmdaq.ChannelMapFile", "",kEnvLocal);
   env.SetValue("mmdaq.PathToPedestalFile", "/data/mmega2010/apv_data/root",kEnvLocal);
   env.SetValue("mmdaq.LogFile", "/data/mmega2010/apv_data/mmdaq.log",kEnvLocal);
   env.SetValue("mmdaq.ZeroTresholdFactor", "10", kEnvLocal);
   env.SetValue("mmdaq.PedestalEventsPerSave", "10", kEnvLocal);
	env.SetRcName("testEnv.txt");

	//m_write_data_path
	//this is ok:
//	env.WriteFile("testEnv.txt", kEnvLocal);

}


void CConfiguration::clear()
{
	m_error = 0;
	m_config_file.clear();
	m_log_file.clear();
	m_pedestal_file.clear();
	m_pedestal_path.clear();
	m_write_data_path.clear();
	m_map_file.clear();
	m_crosstalk_file.clear();
	m_daq_ip_address.clear();
	m_daq_ip_port.clear();
	m_fec_ip_address.clear();
	m_suppresing_zeros = false;
	m_ignore_run = false;
	m_type_of_run = runTypeBad;
	m_run_number = 0;
	m_comments.clear();
	m_number_of_chips = 0;
	
	while (!m_defined_fecs.empty()) {
		CDetFec* fec = m_defined_fecs.front();
		m_defined_fecs.erase(m_defined_fecs.begin());
		delete fec; fec = 0;
	}
	while (!m_defined_chips.empty()) {
		CDetChip* chip = m_defined_chips.front();
		m_defined_chips.erase(m_defined_chips.begin());
		delete chip; chip = 0;
	}
	while (!m_defined_chambers.empty()) {
		CDetChamber* cham = m_defined_chambers.front();
		m_defined_chambers.erase(m_defined_chambers.begin());
		delete cham; cham = 0;
	}
	
	m_defined_chamber_strip_ranges.clear();
	m_lookup_chip_fec.clear();
	m_lookup_fec_chip.clear();
	
	m_lookup_chip_chamber.clear(); 
	m_lookup_chamber_chip.clear();
	m_lookup_chamber_name_chip.clear();
	m_lookup_chip_chamber_name.clear();
	m_log_entries.clear();
	
	m_pedestals_mean.clear();
	m_pedestals_stdev.clear();
	m_sigma_cut_factor = 0;
	m_pedestal_events_per_save = 0;
	m_channel_map.clear();
   m_strip_map.clear();
	m_crosstalk_map.clear();
	m_geo_ApvToChamber.clear();
	
}


/* Handle all command line arguments */
void CConfiguration::HandleArgs(int argc, char **argv)
{
   for (int optNum = 1; optNum != argc;) {
      if (strcmp(argv[optNum], "--ignore") == 0) {
         m_ignore_run = true;
         optNum++;
      }
//      if (strcmp(argv[optNum], "-P") == 0) {
//         m_type_of_run = runtypePedestals;
//         optNum++;
//      }
      if (strncmp(argv[optNum], "-p:", 3) == 0) {
         m_suppresing_zeros = true;
         string str(argv[optNum]);
         m_pedestal_file = str.substr(3);
         cout << "handleargs:" << argv[optNum] << " : " <<  m_pedestal_file << endl;
         optNum++;



      }
      else if (strncmp(argv[optNum], "--config:", 9) == 0) {
         string str(argv[optNum]);
         m_config_file = str.substr(9);
         optNum++;
      }
//      else if (argv[optNum][0] != '-') {//TODo: rdr->
//         m_comments = argv[optNum];
//         optNum++;
//      }
      else {
         usage(argv[0]);
         throw std::string("Unknown arguments at command line.");
      }
   }

} /* End of handleArgs */

const void CConfiguration::usage(const char *ourName) const
{
   cerr << "Usage: " << ourName << " [--config:configuration_file]" << endl;
}



int CConfiguration::load_configuration()
{
   
	std::stringstream thrown;
	bool err = false;
	std::stringstream sserror;
	
   if (m_config_file == "") {
      thrown << "ERROR: load_configuration(): Config file has not been set" << endl;
      throw thrown.str();
      return -1;
   }

   struct stat stFileInfo;
   if (stat(m_config_file.c_str(), &stFileInfo)) {
      thrown << "ERROR: load_configuration(): Config file '" << m_config_file << "' does not exist" << endl;
      throw thrown.str();
      //cerr << "ERROR: load_configuration(): Config file '" << m_config_file << "' does not exist" << endl;
      return -1;
   };

   
   if (m_env) {
      thrown << "ERROR: load_configuration(): TEnv *m_env is already defined. Do not know how to reload yet." << endl;
      throw thrown.str();
   }
   
   size_t size_read = 0;
   //unsigned size_unique = 0;
   TString str;
   m_env = new TEnv(m_config_file.c_str());
   m_daq_ip_address = m_env->GetValue("mmdaq.DAQIPAddress", "127.0.0.1");
   m_daq_ip_port = m_env->GetValue("mmdaq.DAQIPPort", "6006");
   m_map_file = m_env->GetValue("mmdaq.ChannelMapFile", "");
   m_crosstalk_file = m_env->GetValue("mmdaq.CrosstalkCorrectionFile", "");
	m_pedestal_path = m_env->GetValue("mmdaq.PathToPedestalFile", "/data/mmega2010/apv_data/root");
   m_log_file = m_env->GetValue("mmdaq.LogFile", ""); ///data/mmega2010/apv_data/mmdaq.log
   m_sigma_cut_factor = m_env->GetValue("mmdaq.ZeroTresholdFactor", 0.8);
   m_pedestal_events_per_save = m_env->GetValue("mmdaq.PedestalEventsPerSave", 10);
	m_write_data_path = m_env->GetValue("mmdaq.WriteDataPath", "");// mmdaq.WriteDataPath: /data/mmega2011/apv_data/root/

	if (m_map_file.empty()) {
      sserror << "ERROR: load_configuration(): 'mmdaq.ChannelMapFile' not specified" << endl;
      err = true;
	}
	if (m_log_file.empty()) {
      sserror << "ERROR: load_configuration(): 'mmdaq.LogFile' not specified" << endl;
      err = true;
	}
	if (m_write_data_path.empty()) {
      sserror << "ERROR: load_configuration(): 'mmdaq.WriteDataPath' not specified" << endl;
      err = true;
	}
	else {
		//write path -> add trailing slash, test write permission
		if (m_write_data_path.at(m_write_data_path.size() - 1) != '/') {
			m_write_data_path += '/';
		}
		//setup temporary file
		time_t timenow = time(NULL);
		std::stringstream tout;
		tout << m_write_data_path << "mmdaq_tmp_" << timenow;
		std::ofstream testfile( tout.str().c_str(), std::ios_base::trunc);
		if (testfile.good()) {
			testfile.close();
			remove(tout.str().c_str()); //ok can write
		}
		else {
			sserror << "ERROR: cannot write to dir 'm_write_data_path'=" << m_write_data_path << endl;
			err = true;
		}
	}

	
	if (err) {
		thrown << "ERROR: load_configuration(): errors in file '" << m_config_file << "':" <<endl;
		thrown << sserror.str();
//		delete env;
		throw thrown.str();
		return -1;
	}
	
		
	
	
	//detector config 
   str = m_env->GetValue("mmdaq.FEC1.IPAddress", "127.0.0.1");
   FillCSVToVector(str, m_fec_ip_address, ",");
   //FECs
   std::vector<std::string> list_of_fec_names;  //declared fecs
   str = m_env->GetValue("mmdaq.FECs", "");
   FillCSVToVector(str, list_of_fec_names, ",");
   size_read = list_of_fec_names.size();
   MakeElementsUnique(list_of_fec_names);
   if (size_read != list_of_fec_names.size()) {
//		delete env;
      thrown << "ERROR: load_configuration(): 'mmdaq.FECs:'  - not unique values" << endl;
      throw thrown.str();
      return -1;
   }

   int ii = 1;
   for (vector<string>::iterator ifec = list_of_fec_names.begin() ; ifec != list_of_fec_names.end(); ++ifec, ++ii) {
      //TODO: create fec
      CDetFec* afec = new CDetFec(ii, *ifec);
      afec->ipaddress(m_env->GetValue(Form("mmdaq.%s.IPAddress", afec->name().c_str()), ""));
      str = m_env->GetValue(Form("mmdaq.%s.Chips", ifec->c_str()), "");
      vector<string> tmp_fec_chips;
      FillCSVToVector(str, tmp_fec_chips, ","); //TODO: check uniqueness !
      int jj = 0;
      for (vector<string>::iterator ic = tmp_fec_chips.begin(); ic != tmp_fec_chips.end(); ++ic, ++jj) {
         //load mmdaq.FEC1.Chips.APV0.Id: 0
         TString key = Form("mmdaq.%s.Chips.%s.Id", afec->name().c_str(), ic->c_str());
         str = m_env->GetValue(key, "");  //"APV0, APV2"

         int chipId = CApvEvent::make_chipId(afec->get_id(), str.Atoi()); //int chipId = (afec->id() << 4) | str.Atoi();
			
         CDetChip* achip = new CDetChip(chipId, chipId , *ic);
         achip->connect_fec(afec);
         afec->connect(achip);
         m_defined_chips.push_back(achip);


      }
      m_defined_fecs.push_back(afec);
   }
   m_number_of_chips = m_defined_chips.size();


   //Chambers
   std::vector<std::string> list_of_chamber_names;  //declared chambers
   str = m_env->GetValue("mmdaq.Chambers", "");		// "R11, R16"
   FillCSVToVector(str, list_of_chamber_names, ",");
   size_read = list_of_chamber_names.size();
   MakeElementsUnique(list_of_chamber_names);
   if (size_read != list_of_chamber_names.size()) {
//		delete env;
      thrown << "ERROR: load_configuration(): 'mmdaq.Chambers:'  - not unique values" << endl;
      throw thrown.str();
      return -1;
   }
	
   //loop chambers
   int ic = 0;
   for (vector<string>::iterator ichamb = list_of_chamber_names.begin() ; ichamb != list_of_chamber_names.end(); ++ichamb, ++ic) {
      //create chamber
      CDetChamber* achamb = new CDetChamber(ic, *ichamb);
      str = m_env->GetValue(Form("mmdaq.Chamber.%s.Strips", achamb->name().c_str()), ""); //"X, Y, U, V"
      vector<string> tmp_chamb_strips;
      FillCSVToVector(str, tmp_chamb_strips, ","); //TODO: check uniqueness!
		MakeElementsUnique(tmp_chamb_strips);

      //loop strips/readouts : X,Y...
      for (vector<string>::iterator istrip = tmp_chamb_strips.begin(); istrip != tmp_chamb_strips.end(); ++istrip) {
         //mmdaq.Chamber.R16.Strips.X.Angle: 90.0
         TString keyangle = Form("mmdaq.Chamber.%s.Strips.%s.Angle", achamb->name().c_str(), istrip->c_str());
         double strip_angle_deg = m_env->GetValue(keyangle, 0.0);
         std::pair<std::string, double> anglepair = std::pair<std::string, double>(*istrip, strip_angle_deg);
         m_lookup_chamber_readout_angle[achamb->name()].push_back(anglepair);

         TString keypitch = Form("mmdaq.Chamber.%s.Strips.%s.Pitch", achamb->name().c_str(), istrip->c_str());
         double strip_pitch = m_env->GetValue(keypitch, 0.0);
         if (strip_pitch < EPS) {
//            delete env;
            thrown << "Error: Bad value: Strips " << *istrip << " on chamber " << *ichamb << " have strip pitch = 0." << endl;
            throw thrown.str();
         }
         
         TString keyminstrip = Form("mmdaq.Chamber.%s.Strips.%s.Min", achamb->name().c_str(), istrip->c_str());
         double strip_min = m_env->GetValue(keyminstrip, 0.0);
         if (strip_min < EPS) {
//            delete env;
            thrown << "Error: Bad value: Strips " << *istrip << " on chamber " << *ichamb << " have min strip = 0. Should be > 0" << endl;
            throw thrown.str();
         }
         TString keymaxstrip = Form("mmdaq.Chamber.%s.Strips.%s.Max", achamb->name().c_str(), istrip->c_str());
         double strip_max = m_env->GetValue(keymaxstrip, 0.0);
         if (strip_max < EPS || strip_max < strip_min) {
//            delete env;
            thrown << "Error: Bad value: Strips " << *istrip << " on chamber " << *ichamb << " have wrong max range (" << strip_max << ")." << endl;
            throw thrown.str();
         }
         std::pair<double, double> strip_range = std::pair<double, double>(strip_min, strip_max);
         CDetAbstractReadout* rd = 0;
         if ( std::fabs(strip_angle_deg) < EPS ) {
            rd = new CReadoutH(achamb, achamb->get_readouts().size(), *istrip, strip_pitch, strip_range);
         }
         else if ( std::fabs(strip_angle_deg - 90.0) < EPS ) {
            rd = new CReadoutV(achamb, achamb->get_readouts().size(), *istrip, strip_pitch, strip_range);
         }
         else {
            rd = new CReadout(achamb, achamb->get_readouts().size(), *istrip, strip_angle_deg, strip_pitch, strip_range);
         }
         
         //mmdaq.Chamber.R16.Strips.X.Chips: APV2
			TString key = Form("mmdaq.Chamber.%s.Strips.%s.Chips", achamb->name().c_str(), istrip->c_str());
         str = m_env->GetValue(key, "");  //"APV0, APV2"
         vector<string> tmp_chamb_strips_chips;
         FillCSVToVector(str, tmp_chamb_strips_chips, ","); 
			MakeElementsUnique(tmp_chamb_strips_chips);
         
			//loop chips on each readout
         for (vector<string>::iterator ic = tmp_chamb_strips_chips.begin(); ic != tmp_chamb_strips_chips.end(); ++ic) {
            //check that chip has been defined in fec
            CDetChip* achip = find_defined_chip(*ic);
            if (!achip) {
//					delete env;
               thrown << "Error: Chip " << *ic << " on chamber " << *ichamb << " has not been defined in FEC section." << endl;
               throw thrown.str();
               return -1;
            }
            //TODO: load adapter information
            //TODO: later load mapping file
				
            rd->add_chip(achip); 
            
            //attach chip to chamber
            achamb->connect(achip);//, *istrip); //top, on strips
            achip->connect(achamb, CONNECT_BOTTOM); //bottom
            pair<int, int> chip_cham = pair<int, int>(achip->get_id(), achamb->get_id());
            pair<int, int> cham_chip = pair<int, int>(achamb->get_id(), achip->get_id());
            pair<int, string> chip_cham_name = pair<int, string>(achip->get_id(), achamb->name());
            pair<string, int> cham_name_chip = pair<string, int>(achamb->name(), achip->get_id());
            
            m_lookup_chip_chamber.insert(chip_cham);
            m_lookup_chamber_chip.insert(cham_chip);
            m_lookup_chip_chamber_name.insert(chip_cham_name);
            m_lookup_chamber_name_chip.insert(cham_name_chip);
            cout << "m_lookup_chip_chamber fill: " << achip->get_id() << " " <<  achamb->get_id()  << endl;
         }//loop chips
         achamb->add_readout( rd );
      }//loop strips (X,Y)
      m_defined_chambers.push_back(achamb);
   }//loop chambers




   //str = env->GetValue("mmdaq.Chips", "");
//   FillCSVToVector(str, m_list_of_chips, ",");
//

//   delete env;

   if (m_fec_ip_address.size() == 0) {
      thrown << "ERROR: load_configuration(): FEC IP addresses have not been set" << endl;
      throw thrown.str();
      return -1;
   }
	 cout << "loaded mmdaq config file: '" << m_config_file << "'" << endl;

   return 0;

};



void CConfiguration::FillCSVToIntVector(TString &line, vector<int>& vec, const char* delim)
{
   if (!delim) {
      delim = ",";
   }
   //tokenize the rest of string
   TObjArray* strings = line.Tokenize(delim);
   vector<unsigned> values;
   vector<unsigned>::iterator ival;
   if (strings->GetEntriesFast()) {
      TIter istring(strings);
      TObjString* os = 0;
      Int_t j = 0;
      while ((os = (TObjString*)istring())) {
         j++;
         TString s = os->GetString();
         s.Remove(TString::kBoth, ' ');
         vec.push_back(s.Atoi());		//into vector of strings
      }
      //delete os;
   }//if(strings->GetEntriesFast())
   delete strings;

}


void CConfiguration::FillCSVToVector(TString &line, vector<string>& list, const char* delim)
{
   //tokenize the rest of string
   if (!delim) {
      delim = ",";
   }
   TObjArray* strings = line.Tokenize(delim);
   vector<unsigned> values;
   vector<unsigned>::iterator ival;
   if (strings->GetEntriesFast()) {
      TIter istring(strings);
      TObjString* os = 0;
      Int_t j = 0;
      while ((os = (TObjString*)istring())) {
         j++;
         TString s = os->GetString();
         s.Remove(TString::kBoth, ' ');
         list.push_back(string(s.Data()));		//into vector of strings
      }
      //delete os;
   }//if(strings->GetEntriesFast())
   delete strings;

}

int CConfiguration::load_log()
{
   struct stat stFileInfo;
   if (stat(m_log_file.c_str(), &stFileInfo)) {
      cerr << "ERROR: load_log() " << "Log file '" << m_log_file.c_str() << "' does not exist. " << endl;
      cerr << "to create one do: touch " << m_log_file << endl;
      return -1;
   }

   m_log_entries.clear();
   //string filename(config->log_filename());
   std::ifstream infile(m_log_file.c_str());
   while (!infile.eof()) {
      string buff;
      getline(infile, buff);
      m_log_entries.push_back(buff);
   }
   infile.close();
   return 0;
}


int CConfiguration::get_next_run_number_from_log()
{
   if (!m_log_entries.size()) {
      m_run_number = MMDAQ_BAD_RUNNUMBER;
      return m_run_number;
   }
   int lastrun = MMDAQ_LOWEST_RUNNUMBER;
   for (vector<string>::iterator iter = m_log_entries.begin(); iter != m_log_entries.end(); ++iter) {
      TString line(*iter);
      TObjArray* strings = line.Tokenize(",");
      vector<unsigned> values;
      vector<unsigned>::iterator ival;
      if (strings->GetEntriesFast()) {
         TIter istring(strings);
         TObjString* os = (TObjString*)istring();
         TString s = os->GetString();
         s = TString(remove_quotes(s));
         int num = s.Atoi();
         lastrun = std::max(lastrun, num);
      }//if(strings->GetEntriesFast())
      delete strings; strings = NULL;
   }
   m_run_number = lastrun + 1;
   return m_run_number;
}



void CConfiguration::PrintConfig() const
{
   cout << "-------------------------  Configuration -------------------------" << endl;
   cout << "m_config_file:      " << m_config_file << endl;
   cout << "m_daq_ip_address:   " << m_daq_ip_address << endl;
   cout << "m_fec_ip_addresses: ";
   for (vector<string>::const_iterator itf = m_fec_ip_address.begin(); itf != m_fec_ip_address.end(); ++itf) cout << *itf << " ";
   cout << endl;
   cout << "m_log_file:         " << m_log_file << endl;;
//   cout << "m_ignore_run:      " << m_ignore_run << endl;;
//   cout << "m_type_of_run:     " << (m_type_of_run == runtypePhysics ? ("physics") : ("pedestals")) << endl;;
   cout << "(next) m_run_number: " << m_run_number << endl;;
   cout << "m_comments:         " << m_comments << endl;;
   cout << "m_number_of_chips:  " << m_number_of_chips << endl;;
   cout << "m_list_of_chips:    ";
   for (vector<CDetChip*>::const_iterator itc = m_defined_chips.begin(); itc != m_defined_chips.end(); ++itc) cout << (*itc)->name() << " ";
   cout << endl;

   cout << "m_sigma_cut_factor:    " <<m_sigma_cut_factor << endl;
   cout << "m_pedestal_file:    " << m_pedestal_file << endl;
   cout << "m_crosstalk_file:   " << m_crosstalk_file << endl;
	cout << "m_suppresing_zeros: " << m_suppresing_zeros << endl;;
   cout << "m_pedestals_mean_size:  " << m_pedestals_mean.size() << endl;
   cout << "m_pedestals_stdev_size: " << m_pedestals_stdev.size() << endl;
   cout << "m_pedestal_events_per_save: " << m_pedestal_events_per_save  << endl;
   cout << "----------------------- End of Configuration -----------------------" << endl;


   cout << "geometry - fecs" << endl;
   for (std::vector<CDetFec*>:: const_iterator ifec = m_defined_fecs.begin(); ifec != m_defined_fecs.end(); ++ifec) {
      (*ifec)->print();
   }
   cout << endl;
   cout << "geometry - chips" << endl;
   for (std::vector<CDetChip*>:: const_iterator ifec = m_defined_chips.begin(); ifec != m_defined_chips.end(); ++ifec) {
      (*ifec)->print();
   }
   cout << endl;
   cout << "geometry - chambers" << endl;
   for (std::vector<CDetChamber*>:: const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      (*ichamb)->print(); cout << endl;
       std::vector< std::pair<std::string, double> > readouts = get_chamber_readouts( (*ichamb)->name() );
      for ( std::vector< std::pair<std::string, double> > :: const_iterator ird = readouts.begin(); ird != readouts.end(); ++ird) {
         std::cout << " Readout: " << ird->first << " " << ird->second << std::endl;
      }
   }

};

void CConfiguration::pedestal_filename(const char* name)
{
	if (!name) {
		return;
	}
	m_pedestal_file = std::string(name);
}



bool CConfiguration::locate_pedestal_file(std::string& filename) 
{
	if (filename.empty()) {
		return false;
	}
	struct stat stFileInfo;
   if (stat(filename.c_str(), &stFileInfo)) {
      cout << filename << " locate_pedestal_file(): - not a local file, will try default location: " ;
      //no local file, try default location:
      string default_location = filename;
      size_t lastslash = filename.find_last_of("/");
      if (lastslash != string::npos) {
         default_location = filename.substr(lastslash + 1);
      }
      if (m_pedestal_path.at(m_pedestal_path.size() - 1) != '/') {
         m_pedestal_path += '/';
      }
      default_location = m_pedestal_path + default_location;
      cout << default_location << endl;
      if (stat(default_location.c_str(), &stFileInfo)) {
         cerr << "ERROR: locate_pedestal_file(): File does not exist: '" << filename << "'"<<endl;
         cerr << "ERROR: locate_pedestal_file(): File does not exist: '" << default_location <<"'"<< endl;
         return false;
      }
      filename = default_location;
		return true;
   }
	return true; 
}

void CConfiguration::clear_pedestals()
{
   m_pedestal_file.clear();
   m_pedestals_event->clear();
   m_pedestals_mean.clear();
   m_pedestals_stdev.clear();
   m_pedestal_chip_mean_lookup.clear();
   m_pedestal_chip_stdev_lookup.clear();
}


int CConfiguration::load_pedestals(const char* filename)
{
	
//   std::string fname(filename);
   clear_pedestals();
	pedestal_filename(filename);
	
	cout << "load pedestals(): " << m_pedestal_file << endl;

	m_pedestals_event->clear();
   m_pedestals_mean.clear();
	m_pedestals_stdev.clear();

   
   if (!locate_pedestal_file(m_pedestal_file) ){
		return -1;
	}
   
//	cout << "m_pedestal_file = '" << m_pedestal_file << "'" << endl;
   //SplitFileName(m_pedestal_file.c_str(), parts);
   size_t lastdot = m_pedestal_file.find_last_of(".");
   string ext = m_pedestal_file.substr(lastdot);
   if (ext != ".root") {
      cerr << "ERROR: load_pedestals(): Pedestal file should be .root not" <<  ext << endl;
      //m_root_ped = 0;
      return -1;
   }

   //trying NEW version (pedestals tree)
   CApvPedestalsRoot* pedestals_root = new CApvPedestalsRoot(m_pedestal_file.c_str());
	if (pedestals_root && !(pedestals_root->error() < 0 ) ) {
      if (pedestals_root->GetEntries() == 0) {
         cerr << "ERROR: load_pedestals(): Pedestal file is empty (" << m_pedestal_file << ")" << endl;
         delete pedestals_root;
         pedestals_root = 0;
         return -1;
      }
      cout << "Info: load_pedestals(): '" <<m_pedestal_file <<"' has entries=" << pedestals_root->GetEntries() << endl;
     
		
		//read file
      int evt = 0;
      m_pedestals_mean.clear();
      while (pedestals_root->GetEntry(evt++)) {
			
			
			size_t vecsize = pedestals_root->apv_fecNo->size();
			//cout << "vecsize = " << vecsize << endl;
			
			for (int i = 0; i < vecsize; ++i) {
				//string chamber = pedestals_root->mm_id->at(i);
				unsigned fecNo = pedestals_root->apv_fecNo->at(i);
				unsigned apvNo = pedestals_root->apv_id->at(i);
				unsigned apvCh = pedestals_root->apv_ch->at(i);
				int apvChId = CApvEvent::make_channelId(fecNo, apvNo, apvCh);
//				unsigned apvId = ( fecNo << 4 ) | apvNo;
//				long apvChId = (apvId << 8 ) | apvCh;
				
				//long stripNo = pedestals_root->mm_strip->at(i);
				
				double mean = pedestals_root->apv_pedmean->at(i);
            double stdev = pedestals_root->apv_pedstd->at(i);
				double sigma = pedestals_root->apv_pedsigma->at(i);
				m_pedestals_mean[apvChId] = mean; //.push_back((m_root_ped->apv_pedmean)->at(pos));
            m_pedestals_stdev[apvChId] = stdev; //push_back((m_root_ped->apv_pedstd)->at(pos));

				//into CMMEvent
				std::vector<double> pedvec(3,0);
				pedvec[0] = mean;//mean
				pedvec[1] = stdev; //stdev
				pedvec[2] = sigma;//sigma
				m_pedestals_event->add_pedestal(apvChId, pedvec);
			}
			
      }//while getentry()
		
      delete pedestals_root;
      pedestals_root = 0;
		
		//m_pedestals_event->print();
		
      //make cache of mean and stddev pedestal values for chips
      std::for_each(m_defined_chips.begin(), m_defined_chips.end(),
                    boost::bind(&CConfiguration::pedestal_chip_mean_vector, this,
                                boost::bind(&CDetChip::get_id, _1)   ));
      
      return 0;
   } // END NEW version
   cerr << "Info: load_pedestals(): error instantiating CApvPedestalsRoot from " <<  m_pedestal_file << endl;
   return -1;
}

/// get mean pedestal value for the chip
bool CConfiguration::pedestal_chip_mean(unsigned chipId, double& pedestal) const
{
   
   //TODO: add caching map<chipId, ped_val> calculate if not found
   bool found = false;
   std::vector<double> vec;
   vec.reserve(128);
   for (std::map<int, double >::const_iterator iped = m_pedestals_mean.begin();
        iped != m_pedestals_mean.end(); ++iped) {
      if (chipId == CApvEvent::chipId_from_chId(iped->first)) {
         vec.push_back(iped->second);
         found = true;
      }
   }
   pedestal = GetVectorMean(vec);
   return found;
}


///get cached value of chip's pedestal mean over all chip channels
std::vector<double> const& CConfiguration::pedestal_chip_mean_vector(int chipId)
{
   std::map<int, std::vector<double> > :: iterator found = m_pedestal_chip_mean_lookup.find(chipId);
   if (found == m_pedestal_chip_mean_lookup.end()) {
      //make it
      
      std::vector<double> vec(128);
      for (std::map<int, double >::const_iterator iped = m_pedestals_mean.begin();
           iped != m_pedestals_mean.end(); ++iped) {
         int apvid = CApvEvent::chipId_from_chId(iped->first);
         int chid = CApvEvent::chanNo_from_chId(iped->first);
         if (chipId == apvid) {
            vec[chid] = iped->second;
            //std::cout << "pedestal_chip_mean =" << chipId << ":: " <<  CApvEvent::chipId_from_chId(iped->first) 
            //<< " >>> " <<iped->first << " -> " << CApvEvent::chanNo_from_chId(iped->first) << " : " << iped->second << std::endl;
//            std::cout << "CConfiguration::pedestal_chip_mean_vector: " << iped->first << "->" << CApvEvent::chanNo_from_chId(iped->first) << ":" << iped->second << std::endl;
         }
      }
      found = m_pedestal_chip_mean_lookup.insert(std::pair<int, std::vector<double> >(chipId, vec) ).first;
   }
   return found->second;   
}


///get cached value of chip's pedestal stddev over all chip channels
std::vector<double> const& CConfiguration::pedestal_chip_stdev_vector(int chipId)
{
   std::map<int, std::vector<double> > :: iterator found = m_pedestal_chip_stdev_lookup.find(chipId);
   if (found == m_pedestal_chip_stdev_lookup.end()) {
      //make it
      
      std::vector<double> vec(128);
      for (std::map<int, double >::const_iterator iped = m_pedestals_stdev.begin();
           iped != m_pedestals_stdev.end(); ++iped) {
         if (chipId == CApvEvent::chipId_from_chId(iped->first)) {
            vec[CApvEvent::chanNo_from_chId(iped->first)] = iped->second;
         }
      }
      found = m_pedestal_chip_stdev_lookup.insert(std::pair<int, std::vector<double> >(chipId, vec) ).first;
   }
   return found->second;
}


const bool CConfiguration::pedestal_mean(const unsigned chId, double& pedestal) const
{
   //double val = 0.0;
   std::map<int, double >::const_iterator iped = m_pedestals_mean.find(chId);
   if (iped != m_pedestals_mean.end()) {
      pedestal = iped->second;
      return true;
   }
   //cout << "pedestal_mean() : not found :" << chId << " ( " << (chId &0xFF) << " on " << chip_no(chId) << endl;
   return false; //chId not found
};


const bool CConfiguration::pedestal_stdev(const unsigned chId, double& value) const
{
   //double val = 0.0;
   std::map<int, double >::const_iterator iped = m_pedestals_stdev.find(chId);
   if (iped != m_pedestals_stdev.end()) {
      value = iped->second;
      return true;
   }
   return false; //chId not found
};



int CConfiguration::load_channel_map()
{

   int err = 0;

   struct stat stFileInfo;
   if (stat(m_map_file.c_str(), &stFileInfo)) {
      cerr << "load_channel_map(): Map file '"<< m_map_file << "' does not exist." << endl;
      return -1;
   }

   TEnv* env = new TEnv(m_map_file.c_str());
   TString str;
   str = env->GetValue("APVid", "");
   if (str.Length() == 0) {
		delete env;
      cerr << "load_channel_map(): Missing keyword: APVid (list of APV chips)" << endl;
      return -1;
   }
   vector<int> geo_apv_list;
   FillCSVToIntVector(str, geo_apv_list, " \t");

   str = env->GetValue("Chamber", "");
   if (str.Length() == 0) {
		delete env;
      cerr << "LoadChannelMapFile: Missing keyword: Chamber (list of Chambers)" << endl;
      return -1;
   }
   vector<string> geo_chamber_list;
   FillCSVToVector(str, geo_chamber_list, " \t");

   delete env;


	
	//TODO: check if map corresponds to declared chips
	

   int fecNo = 1;
   int ii = 0;
   //	cout << "Info: ";
   for (vector<int>::iterator ichip = geo_apv_list.begin(); ichip != geo_apv_list.end(); ++ichip, ++ii) {
      //		cout << *ichip << "(" << geo_chamber_list[ii] <<") ";
		Int_t apvId = CApvEvent::make_chipId(fecNo, *ichip); // Int_t apvId = (fecNo << 4) | *ichip;
      m_geo_ApvToChamber[apvId] = geo_chamber_list[ii];
   }
   //	cout << endl;


   vector <int> geo_strip_ranges_hi(geo_chamber_list.size(), 0);
   vector <int> geo_strip_ranges_lo(geo_chamber_list.size(), 0);


   size_t numberOfValues = 0; //should be const number of values per line
   int numberOfLinesWithValues = 0; //should be 128 (0-127)
   //read while eof
   //push vector of values into map

   std::ifstream file(m_map_file.c_str(), std::ifstream::in);
   if (!file.is_open()) {
      cerr << "LoadChannelMapFile: Error loading " << m_map_file << endl;
      return -1;
   }

   // Info("LoadChannelMapFile", "Reading %s", gSystem->ExpandPathName(filename));


   int lineCounter = 0;
   char buff[256];


   while (file.getline(buff, 256)) {
      TString line(buff);
      //      if (!lineCounter) {
      //			cout << "Info: Channel map file 1st line:   ";
      //         cout << line.Data() << endl;
      //		}
      lineCounter++;
      line.Remove(TString::kLeading, ' ');
      if (line.BeginsWith("#")) continue;
      if (!line.Length()) continue;
      if (line.Contains(":")) {
         //check if looks like keyword then continue
         continue;
      }


      //tokenize the rest of string
      TObjArray* strings = line.Tokenize(" \t");
      vector<int> values;
      vector<unsigned>::iterator ival;
      if (strings->GetEntriesFast()) {
         TIter istring(strings);
         TObjString* os = 0;
         Int_t j = 0;
         while ((os = (TObjString*)istring())) {
            j++;
            TString s = os->GetString();
            s.ReplaceAll("\t", " ");
            s.Remove(TString::kBoth, ' ');
            int value = s.Atoi();
            if (!s.Length()) {
               value = -1;
            }
            values.push_back(value);		//into vector of strings
         }
      }//if(strings->GetEntriesFast())
      delete strings;

      if (values.size() < 2) {
         cerr << "LoadChannelMapFile: not enough columns " << values.size() << " in line " << lineCounter << endl;
         cerr << "LoadChannelMapFile: Error in " << m_map_file << endl;
         err = -1;
         return err;
      }
      else if (numberOfLinesWithValues > 1 && values.size() !=  numberOfValues) {
         cerr << "LoadChannelMapFile: number of columns" << values.size() << " in line " << lineCounter << " (ch:" << values[0] << ") differs from previous line" << endl;
         err = -1;
      }
      numberOfValues = values.size();


      //unsigned channel = values.front();
      int jj = 0;
      int fecNum = 1;
      //get max and min strip number for each chip column
      for (vector<int>::iterator ivalue = values.begin() + 1; ivalue != values.end() ; ++ivalue, ++jj) {
         int chipnumber = geo_apv_list[jj];
			Int_t apvIdCh = CApvEvent::make_channelId(fecNum, chipnumber, values.front()); //Int_t apvIdCh = (((fecNum << 4) | chipnumber) << 8) | values.front();
         m_channel_map[apvIdCh] = *ivalue + 1; //+1 here for error check in CRawWriter::GetMMStripFromAPVChannel()
         m_strip_map[*ivalue] = apvIdCh;
         if (!geo_strip_ranges_hi[jj] && (*ivalue > 0))
            geo_strip_ranges_hi[jj] = *ivalue;
         if (!geo_strip_ranges_lo[jj] && (*ivalue > 0))
            geo_strip_ranges_lo[jj] = *ivalue;
         if ((geo_strip_ranges_hi[jj] < *ivalue && (*ivalue > 0)))
            geo_strip_ranges_hi[jj] = *ivalue;
         if ((geo_strip_ranges_lo[jj] > *ivalue) && (*ivalue > 0))
            geo_strip_ranges_lo[jj] = *ivalue;
      }
      //cout << "Loading map for channel " << channel << " in " << fchannelsMapped.size() << " APVs" << endl;
      numberOfLinesWithValues++;
   }//while (line.ReadLine(file))

   //get the max and min strips for the chambers - specified in columns per apv in map file
   vector<string> geo_chamber_list_unique(geo_chamber_list);
   MakeElementsUnique(geo_chamber_list_unique);
   vector<int> geo_strip_ranges_lo_single(geo_chamber_list_unique.size(), 0);
   vector<int> geo_strip_ranges_hi_single(geo_chamber_list_unique.size(), 0);
   int c1 = 0;
   for (vector<string> :: iterator ichamb =  geo_chamber_list_unique.begin(); ichamb != geo_chamber_list_unique.end(); ++ichamb, ++c1) {
      int c2 = 0;
      for (vector<string> :: iterator ichamb2 = geo_chamber_list.begin(); ichamb2 != geo_chamber_list.end(); ++ichamb2, ++c2) {
         if (*ichamb == *ichamb2) {
            //fill vec
            if (!geo_strip_ranges_hi_single[c1] && geo_strip_ranges_hi[c2] > 0)  geo_strip_ranges_hi_single[c1] = geo_strip_ranges_hi[c2];
            if (!geo_strip_ranges_lo_single[c1] && geo_strip_ranges_lo[c2] > 0)  geo_strip_ranges_lo_single[c1] = geo_strip_ranges_lo[c2];
            if (geo_strip_ranges_hi[c2] > 0) geo_strip_ranges_hi_single[c1] = std::max(geo_strip_ranges_hi_single[c1], geo_strip_ranges_hi[c2]);
            if (geo_strip_ranges_lo[c2] > 0) geo_strip_ranges_lo_single[c1] = std::min(geo_strip_ranges_lo_single[c1], geo_strip_ranges_lo[c2]);
         }
      }
   }

   //TODO: fill 	m_defined_chamber_strip_ranges
   c1 = 0;
   for (std::vector<CDetChamber*> :: iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb, ++c1) {
      pair<unsigned, unsigned> thelimits = pair<unsigned, unsigned>(geo_strip_ranges_lo_single[c1], geo_strip_ranges_hi_single[c1]);
      m_defined_chamber_strip_ranges.insert(pair<int, pair<unsigned, unsigned> > ((*ichamb)->get_id(), thelimits));

      cout << "RANGES: id:" << (*ichamb)->get_id() << " " << geo_strip_ranges_lo_single[c1] << " " << geo_strip_ranges_hi_single[c1] << endl;
   }



   cout << "Info: APVs: ";
   for (std::map <unsigned, std::string>::iterator ichm = m_geo_ApvToChamber.begin(); ichm != m_geo_ApvToChamber.end(); ++ichm) {
      int apvId = ichm->first;

      cout << CApvEvent::fecNo_from_chId(apvId) << "_" << CApvEvent::chipNo_from_chipId(apvId) <<  "(" << ichm->second << ") ";

   }
   cout << endl;


   if (m_channel_map.size() != NUMBEROFAPVCHANNELS*geo_apv_list.size() && numberOfLinesWithValues != NUMBEROFAPVCHANNELS) {
      cerr << "LoadChannelMapFile" << m_channel_map.size() << " of channels mapped after reading " << numberOfLinesWithValues << " lines" << endl;
      err = -1;
   }

   if (err)
      cerr << "LoadChannelMapFile: Fatal: Check mapping file " << m_map_file << endl;
   else
      cout << "Info: Channel map file: " << m_map_file << endl;

//   	cout << "THE CHANNEL MAP " <<endl;
//   	for (std::map<unsigned, int > ::iterator ich = m_channel_map.begin(); ich != m_channel_map.end(); ++ich) {
//   		cout << "| apvId:"	<<  chip_id(ich->first) 
//			<< " fecNo:"		<<  fec_no(ich->first)
//			<< " apvNo:"		<<  chip_no(ich->first)
//			<< " ch: "			<<  chan_no(ich->first)
//			<< " -> mm:"		<<  ich->second << endl;
//   	}


   return err;




}




int CConfiguration::load_crosstalk_map()
{

	if (m_crosstalk_file.empty()) {
		return 0;
	}
	
   int err = 0;
   struct stat stFileInfo;
   if (stat(m_crosstalk_file.c_str(), &stFileInfo)) {
      cout << "load_crosstalk_map(): File '"<< m_crosstalk_file << "' does not exist." << endl;
      return -1;
   }
	
	
	/////////
   size_t numberOfValues = 0; //should be const number of values per line
   int numberOfLinesWithValues = 0; //should be 128 (0-127)
   //read while !eof
   //push vector of values into map
	
   char* fullpath = gSystem->ExpandPathName(m_crosstalk_file.c_str()); //must be deleted
   
   std::ifstream crossfile(fullpath, std::ifstream::in);
   if (!crossfile.is_open()) {
      cerr << "load_crosstalk_map(): Error loading " <<  fullpath << endl;
      delete fullpath;
      fullpath = 0;
      return -1;
   }
	
   int lineCounter = 0;
   char buff[256];
   while (crossfile.getline(buff, 256)) {
      TString line(buff);
      lineCounter++;
      line.Remove(TString::kLeading, ' ');
      if (line.BeginsWith("#")) continue;
      if (!line.Length()) continue;
		
      //tokenize the rest of string
      TObjArray* strings = line.Tokenize(" \t");
      vector<int> values;
      vector<int>::iterator ival;
      if (strings->GetEntriesFast()) {
         TIter istring(strings);
         TObjString* os = 0;
         Int_t j = 0;
         while ((os = (TObjString*)istring())) {
            j++;
            TString s = os->GetString();
            int value = s.Atoi();
            values.push_back(value);		//into vector of strings
         }
         //delete os;
      }//if(strings->GetEntriesFast())
      delete strings;
		
      if (values.size() < 3) {
         cerr  << "load_crosstalk_map(): not enough (" << values.size() << ") columns in line " << lineCounter << endl;
         err = -1;
      }
      else if (numberOfLinesWithValues > 1 && values.size() !=  numberOfValues) {
         cerr << "load_crosstalk_map(): number of columns (" << values.size() << ") in line " << lineCounter << " differs from previous line" << endl;
         err = -1;
      }
      numberOfValues = values.size();
      numberOfLinesWithValues++;
      unsigned channel = values.front();
      if (values[1] < 0) {
			continue; //ignore channels without crosstalk
		}

		for (vector<int> ::iterator iv = values.begin(); iv != values.end(); ++iv) {
			m_crosstalk_map[channel].push_back(*iv);
		}
      
   }//while (line.ReadLine(file))
	
   if (err)
      cerr << "load_crosstalk_map(): Fatal: Check crosstalk mapping file '" 
		<< fullpath << "'" << endl;
   else
      cout << "Info: load_crosstalk_map(): Crosstalk file:" << m_crosstalk_file 
		<< " loaded for " << m_crosstalk_map.size() << " channels" << endl;
   delete fullpath;
   fullpath = 0;
   return err;	
}



const int CConfiguration::strip_from_channel(const int apvIdCh) const
{
   //strips in map ar offset by +1 for error checking
   // int strip = -1;
   std::map<unsigned, int >::const_iterator it = m_channel_map.find(apvIdCh);
   if (it != m_channel_map.end()) {
      return it->second;
   }

   //cerr << "not mapped: apvId:" << chip_no(apvIdCh) << " ch: " << chan_no(apvIdCh)  << " -> mm:" << strip << endl;
   return -1;
}


int CConfiguration::channel_from_strip(const int stripId) const
{
   std::map<unsigned, int >::const_iterator it = m_strip_map.find(stripId);
   if (it != m_strip_map.end()) {
      return it->second;
   }
   return -1;
}


const int CConfiguration::strip_from_channel(const int apv_id, const int chNo) const
{
	int apvIdCh = CApvEvent::make_channelId(apv_id, chNo);
	//int apvIdCh = (apv_id << 8) | chNo;
   //strips in map ar offset by +1 for error checking
   // int strip = -1;
   std::map<unsigned, int >::const_iterator it = m_channel_map.find(apvIdCh);
   if (it != m_channel_map.end()) {
      return it->second;
   }
	
   //cerr << "not mapped: apvId:" << chip_no(apvIdCh)  << " ch: " <<  chan_no(apvIdCh) << " -> mm:" << strip << endl;
   return -1;
}


std::vector<std::string> CConfiguration::chip_list_names() const
{
   return defined_chip_names();
}

std::vector<std::string >  CConfiguration::defined_fec_names() const
{
   std::vector<std::string> names(m_defined_fecs.size(), string());
   int ii = 0;
   for (std::vector<CDetFec*>::const_iterator ifec = m_defined_fecs.begin(); ifec != m_defined_fecs.end(); ++ifec, ++ii) {
      names[ii] = (*ifec)->name();
   }
   return names;
}

std::vector<std::string >  CConfiguration::defined_fec_ip_addresses() const
{
   std::vector<std::string> ips(m_defined_fecs.size(), string());
   int ii = 0;
   for (std::vector<CDetFec*>::const_iterator ifec = m_defined_fecs.begin(); ifec != m_defined_fecs.end(); ++ifec, ++ii) {
      ips[ii] = (*ifec)->ipaddress();
   }
   return ips;
}


std::vector<std::string >  CConfiguration::defined_chip_names() const
{
   std::vector<std::string> names(m_defined_chips.size(), string());
   int ii = 0;
   for (std::vector<CDetChip*>::const_iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip, ++ii) {
      names[ii] = (*ichip)->name();
   }
   return names;
}

std::vector<int>  CConfiguration::defined_chip_ids() const
{
   std::vector<int> ident(m_defined_chips.size(), 0);
   int ii = 0;
   for (std::vector<CDetChip*>::const_iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip, ++ii) {
      ident[ii] = (*ichip)->get_id();
   }
   return ident;

}


std::vector<std::string >  CConfiguration::defined_chamber_names() const
{
   std::vector<std::string> names(m_defined_chambers.size(), string());
   int ii = 0;
   for (std::vector<CDetChamber*>::const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb, ++ii) {
      names[ii] = (*ichamb)->name();
   }
   return names;
}

std::string CConfiguration::defined_chamber_name(const int chamber_id) const
{
   for (std::vector<CDetChamber*>::const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      if ((*ichamb)->get_id() == chamber_id) {
         return (*ichamb)->name();
      }
   }
   return string();
}

const std::vector< CDetChip*> CConfiguration::chips_on_chamber(const int chamber_id)
{
   vector< CDetChip*> thechips;
   pair<multimap<int, int>::iterator, multimap<int, int>::iterator> rng = m_lookup_chamber_chip.equal_range(chamber_id);
   for (multimap<int, int>::iterator ipair = rng.first; ipair != rng.second; ++ipair) {
      thechips.push_back(find_defined_chip(ipair->second));
   }
   return thechips;
}



const CDetChip* const CConfiguration::find_defined_chip(const string name) const
{
   for (std::vector<CDetChip*> :: const_iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip) {
      CDetChip* achip = *ichip;
      if (name == achip->name()) {
         return *ichip;
      }
   }
   return 0;
}

CDetChip* CConfiguration::find_defined_chip(const string name)
{
   for (std::vector<CDetChip*> :: iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip) {
      CDetChip* achip = *ichip;
      if (name == achip->name()) {
         return *ichip;
      }
   }
   return 0;
}


const CDetChip* const CConfiguration::find_defined_chip(const int chip_id) const
{
   for (std::vector<CDetChip*> :: const_iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip) {
      CDetChip* achip = *ichip;
      if (chip_id == achip->get_id()) {
         return *ichip;
      }
   }
   return 0;
}

CDetChip* CConfiguration::find_defined_chip(const int chip_id)
{
   for (std::vector<CDetChip*> :: iterator ichip = m_defined_chips.begin(); ichip != m_defined_chips.end(); ++ichip) {
      CDetChip* achip = *ichip;
      if (chip_id == achip->get_id()) {
         return *ichip;
      }
   }
   return 0;
}

const CDetChamber* const CConfiguration::chamber_by_name(const std::string name) const
{
   for (std::vector<CDetChamber*> :: const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      CDetChamber* achamb = *ichamb;
      if (name == achamb->name()) {
         return *ichamb;
      }
   }
   return 0;
}

const CDetChamber* const CConfiguration::get_chamber_by_chipId(int chipId) const
{
   for (std::vector<CDetChamber*> :: const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      if ((*ichamb)->get_readout_for_chipId(chipId)) {
         return *ichamb;
      }
   }
   return 0;
}

CDetChamber* CConfiguration::find_defined_chamber(const int chamb_id)
{
   for (std::vector<CDetChamber*> :: iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      CDetChamber* achamb = *ichamb;
      if (chamb_id == achamb->get_id()) {
         return *ichamb;
      }
   }
   return 0;
}

const CDetChamber* const CConfiguration::find_defined_chamber(const int chamb_id) const
{
   for (std::vector<CDetChamber*> :: const_iterator ichamb = m_defined_chambers.begin(); ichamb != m_defined_chambers.end(); ++ichamb) {
      CDetChamber* achamb = *ichamb;
      if (chamb_id == achamb->get_id()) {
         return *ichamb;
      }
   }
   return 0;
}

const int CConfiguration::chamber_id_by_chip(const int chip_id) const
{
   //cout << "m_lookup_chip_chamber" <<endl;
   for (std::multimap <int, int> :: const_iterator im = m_lookup_chip_chamber.begin(); im != m_lookup_chip_chamber.end(); ++im) {
      if (im->first == chip_id) {
         return find_defined_chamber(im->second)->get_id() ;
      }
   }
   //cout << "not found " << chip_id<< endl;
   return 0;
}

const std::string CConfiguration::chamber_name_by_chip(const int chip_id) const
{
   //cout << "m_lookup_chip_chamber" <<endl;
   for (std::multimap <int, string> :: const_iterator im = m_lookup_chip_chamber_name.begin(); im != m_lookup_chip_chamber_name.end(); ++im) {
      if (im->first == chip_id) {
         //cout << "found " << im->first << " " << im->second << endl;
         return im->second;
      }
   }
   //cout << "not found " << chip_id<< endl;

   return string();
}


std::pair<unsigned, unsigned>   CConfiguration::defined_chamber_strip_ranges(int chamber_id) const
{

//   std::pair<unsigned, unsigned> thepair = pair<unsigned, unsigned>(0, 0) ;

   std::map <int, std::pair<unsigned, unsigned> > :: const_iterator itf = m_defined_chamber_strip_ranges.find(chamber_id);
   if (itf != m_defined_chamber_strip_ranges.end()) {
      return itf->second;
   }
   return  std::pair<unsigned, unsigned> () ;
}



