/*
 ** mmdaq-simple.cpp -- UDP "listener" + file writer

 * threads:
	0. MAIN :
		- UDP socket, bind,

		- close

	1. UDP recvfrom loop
		- pushes data as element to the vector
		- locks: data vector  for writing
		- ends on thread_

	2. FILE WRITER
		- reads first element of the vector ( copy the data), remove from vector
		- locks: data vector  for reading, file  for writing
		- ends on thread_

 */

//#include "CLogger.h"
//#include "CRawWriter.h"
//#include "CReceiver.h"
//#include "CConfiguration.h"
#include "CMMDaq.h"
#include "CUserInterface.h"
#include "MBtools.h"


#include <TApplication.h>

//#include "TString.h"
//#include <TObjArray.h>
//#include <TObjString.h>
//
//#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <errno.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/stat.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <netdb.h>
//
//#include <pthread.h>
//
//#include <cstring> //memcopy
//#include <vector>
//#include <queue>
//#include <string>
//#include <fstream>
//#include <utility>
////#include <time.h>
//#include <cassert>
//#include <iomanip>
//#include <ctime>

//#include <functional>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::list;
using std::vector;

//#define DEBUG

/* flag to denote if the user requested to cancel the operation in the middle, 0 means 'no'. */
//int cancel_operation = 0;


//#define MAXHOSTNAME 256
//#define MMDAQ_LOG_FILENAME "/data/mmega2010/apv_data/mmdaq.log"
//#define MMDAQ_LOG_PATH "/data/mmega2010/apv_data/"

// other for date compatibility:
//#define DATE_SRS_EQ_HDR_ELEMENT_SIZE    ((int32_t) 4)
//int gsockfd = 0;
//bool g_ignore = false;
//std::string g_comments = "";
//into config
//CLogger::runtype_t g_run_type = CLogger::runtypeData; 


int main(int argc, char * argv[])
{

	TApplication *myapp = new TApplication("BrowseEvents", &argc, argv);
	
	CMMDaq *daq = 0;
	try {
		daq = new CMMDaq(argc, argv);
	}
	catch (std::string str) {
		std::cout << str << std::endl;
		std::cout << "Errors in MMDAQ start up - aborting"<< std::endl;
		return EXIT_FAILURE;
	}
	 
	//daq.run();
		
	myapp->Run();
	
	
	delete daq;
	
   return EXIT_SUCCESS;
}


