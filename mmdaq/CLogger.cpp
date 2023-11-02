/*
 *  CLogger.cpp
 *  mmdaq_simple
 *
 *  Created by Marcin Byszewski on 1/24/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CConfiguration.h"
#include "CLogger.h"
#include "CRootWriter.h"


#include <TString.h>
#include <TObjArray.h>
#include <TObjString.h>

#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>


using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::ofstream;


CLogger::CLogger(CConfiguration* config)
: m_config(config), m_logfile(), m_logger_runlist(0),// m_writer(0), 
m_save_data_flag(false)
{
	m_logfile = new std::ofstream( config->log_filename(), std::ios_base::app);
   if (m_logfile->bad()) {
      cerr << "ERROR: CLogger():CLogger() while opening log file for writing. Exit." << endl;
      exit(EXIT_FAILURE);
		//TODO: throw error and cout data about run
		//TODO: or set error flag and do the same
   }	
}



CLogger::~CLogger()
{
	if (m_logfile) {
		m_logfile->close();
		delete m_logfile; m_logfile = 0;
	}
};

int CLogger::close()
{
	m_logfile->close();
	delete m_logfile;
	m_logfile = 0;
	return 0;
};



int CLogger::write(int runnumber, time_t time, int number_of_events, std::string comments)
{
	if (!m_save_data_flag) {
		return 0;
	}
	char buff[64];
	struct tm* timeinfo = localtime ( &time );
	strftime(buff, 64, "%d-%b-%Y\", \"%H:%M ", timeinfo);
//	int number_events = -1;
//	if(m_writer)
//		 number_events = m_writer->event_count();
	string typestring;
	if (m_config->run_type() == CConfiguration::runtypePhysics) typestring = "physics";
	else if (m_config->run_type() == CConfiguration::runtypePedestals) typestring = "pedestals";
	else typestring = "unknown";

	*m_logfile << "\"" << runnumber << "\", \"" << buff << "\", \"" << number_of_events << "\", \"" << typestring << "\", \""<< comments << "\"" << endl;
	return 0;
}
