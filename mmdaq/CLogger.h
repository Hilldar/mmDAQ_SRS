/*
 *  CLogger.h
 *  mmdaq_simple
 *
 *  Created by Marcin Byszewski on 1/24/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CLogger_h
#define CLogger_h

#include <TString.h>

#include <string>
#include <vector>
#include <fstream>


class CConfiguration;
class CRootWriter;
class CLogger;

class CLogger {
	
public:
	//typedef enum runtype_t { runtypeData = 0,	runtypePedestals = 1 };

public:
	CLogger(CConfiguration* config);
	~CLogger();
	
	int close();
//	int init();
//	int get_last_run_number();
	int write(int runnumber, time_t starttime, int number_of_events, std::string comments);
	//void attach_writer(CRootWriter* wrt) { m_writer = wrt; };
	void set_save_data_flag(bool state) { m_save_data_flag = state;} 
private:
	CLogger(const CLogger&);
	CLogger& operator=(const CLogger&);
private:
	//logger members
	CConfiguration*				m_config;
	std::ofstream*					m_logfile;
	std::vector <std::string>	m_logger_runlist; //TODO: change string to RunInfo
	//CRootWriter*					m_writer;
	bool								m_save_data_flag;

	//runtype_t m_runtype;

	

};

#endif
