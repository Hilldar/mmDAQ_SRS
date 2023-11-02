/*
 *  CReceiver.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/26/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CReceiver.h"
#include "CConfiguration.h"
#include "CUDPData.h"
#include "MBtools.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>
#include <netdb.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h> //perror


#define MAXBUFLEN 65507
/*max udpsize is 65507 bytes -> 16377 ints*/

#define MMDAQ_INT32_SIZE 4
#define MMDAQ_INT16_SIZE 2

#ifndef SRS_NEXT_EVENT
#define SRS_NEXT_EVENT 0xFAFAFAFA
#endif


using std::cout;
using std::cerr;
using std::endl;
using std::stringstream;




//CReceiver::CReceiver(CConfiguration* config, std::list<CUDPData*>* datacont, pthread_mutex_t& data_mutex, pthread_cond_t& cond)

CReceiver::CReceiver(CConfiguration* config, std::list<CUDPData*>* datacont, CMutex* recv_data_mutex, pthread_cond_t& cond)
      : m_config(config), m_datacontainer(datacont), m_recv_mutex(recv_data_mutex), /*m_data_mutex(&data_mutex),*/
		m_action_event_received(&cond),
      m_socket(0), m_daq_ip_address(m_config->daq_ip_address()), m_daq_ip_port(m_config->daq_ip_port()),  
		m_fec_ip_address(m_config->defined_fec_ip_addresses()),
		m_event_ready(false), m_fail_message("no message"), m_pause_run(false), m_pause_run_requested(false),
m_ignored_events_counter(0), m_received_events_counter(0), m_internal_queue_size(0)
{ 
	set_thread_name("m_receiver");
}


CReceiver::~CReceiver()
{ 
	m_recv_mutex->lock( thread_id(), thread_name() ,"~Recv()");
	//lock_mutex(m_data_mutex);
	while (!m_datacontainer->empty()) {
		CUDPData* udp = m_datacontainer->front();
		m_datacontainer->erase(m_datacontainer->begin());
		delete udp; udp = 0;
	}
	m_recv_mutex->unlock();
//	unlock_mutex(m_data_mutex);	
}


void* CReceiver::get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }
   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int CReceiver::init()
{
   struct in_addr iaddr;
   inet_pton(AF_INET, m_daq_ip_address.c_str(), &iaddr);

   char buf[128];
   inet_ntop(AF_INET, &iaddr, buf, 128);
//   printf("requested local address = %s .. ", buf);

   struct sockaddr_in sa;       /* Internet address struct            */

   /* initiate machine's Internet address structure */
   /* first clear out the struct, to avoid garbage  */
   memset(&sa, 0, sizeof(sa));
   /* Using Internet address family */
   sa.sin_family = AF_INET;
   /* copy port number in network byte order */
   sa.sin_port = htons(StringToNumber<int>(m_daq_ip_port)) ; //SRSPORT_INT); ;
   /* we will accept connections coming through any IP */
   /* address that belongs to our host, using the      */
   /* INADDR_ANY wild-card.                            */
   sa.sin_addr.s_addr = INADDR_ANY;
   sa.sin_addr   = iaddr;

   struct addrinfo hints, *servinfo, *p;
   int rv;


   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET; // set to AF_INET to force IPv4
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_protocol = 0;

   hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_CANONNAME; // use my IP
   hints.ai_addr = (struct sockaddr*)(&sa);


   // http://linux.die.net/man/3/getaddrinfo
   // The ai_canonname field of the first of these addrinfo structures is set to
   // point to the official name of the host, if hints.ai_flags includes the AI_CANONNAME flag.
   // ai_family, ai_socktype, and ai_protocol specify the socket creation parameters.
   // A pointer to the socket address is placed in the ai_addr member,
   // and the length of the socket address, in bytes, is placed in the ai_addrlen member.

   if ((rv = getaddrinfo(m_daq_ip_address.c_str(), m_daq_ip_port.c_str(), &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		m_fail_message = "failed to getaddrinfo()";
      return -1;
   }

//   // check: print out server data
//   	for (p = servinfo; p != NULL; p = p->ai_next) {
//   		printf("servinfo: fami:%d sock:%d prot:%d\n", p->ai_family, p->ai_socktype, p->ai_protocol);
//   		struct sockaddr_in* sain = (struct sockaddr_in*) p->ai_addr;
//   		inet_ntop(AF_INET, &sain->sin_addr, buf, 128);
//   		printf("servinfo: cano:%s %s\n", p->ai_canonname, buf);
//   	}


   // loop through all the results and bind to the first we can
   for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((m_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         perror("WARNING: CReceiver::init(): socket");
         continue;
      }
      if (bind(m_socket, p->ai_addr, p->ai_addrlen) == -1) {
         close(m_socket);
         perror("WARNING: CReceiver::init(): bind");
         continue;
      }
      break;
   }

   if (p == NULL) {
      cerr << "\rERROR CReceiver::init(): failed to bind socket" << endl;
      cerr << "\rERROR:Check your network configuration. MMDAQ configured on " << m_daq_ip_address << ":" << m_daq_ip_port << " failed to start." << endl;
		m_fail_message = "failed to bind socket - check network";
		close(m_socket);
      return -1;
   }

   //what we have?
   struct sockaddr_in* sas = (struct sockaddr_in*) p->ai_addr;
   struct in_addr* saddr = &sas->sin_addr;
   inet_ntop(AF_INET, saddr, buf, 128);
   freeaddrinfo(servinfo);

	for (std::vector< std::string> :: iterator ifec = m_fec_ip_address.begin(); ifec != m_fec_ip_address.end(); ++ifec) {
		//test ping to hardware -must use - otherwise packet loss
		//TODO: gui status bar
		cout << " CHECK: Sending ping to " << *ifec ;
		stringstream ss;
		ss << "ping -W5 -c 1 " << ifec->c_str() << "> /dev/null 2> /dev/null" << "\0";
		int ret = 0;
		ret = system(ss.str().c_str());
		if (ret != 0) {
			cout << endl;
			cerr << "ERROR: the readout board at " <<  *ifec << " is not responding to ping. Exit." << endl;
			stringstream ssmsg;
			ssmsg << "Failed ping to FEC " <<  *ifec;
			m_fail_message = ssmsg.str();
			close(m_socket);
			return -1;
		} else {
         cout << " OK" << std::endl;
      }
	}
	
   return 0;
}


int CReceiver::main(void*)
{

	unsigned long framecounter = 0;
   
	struct timeval in_time;
   
   while (1) {
      socklen_t addr_len;
      struct sockaddr_storage their_addr;
		
      size_t numbytes;
      unsigned char buf[MAXBUFLEN];

      memset(buf, '\0', MAXBUFLEN);

      addr_len = sizeof their_addr;
      if ((numbytes = recvfrom(m_socket, buf, MAXBUFLEN - 1 , 0, (struct sockaddr *) & their_addr, &addr_len)) == -1) {
         perror("recvfrom");
			m_fail_message = "recvfrom() error";
         exit(EXIT_FAILURE);
      }
		gettimeofday(&in_time, NULL);
		char ipstr[INET6_ADDRSTRLEN];
		char* ipaddr = get_ip_str( (const struct sockaddr *)&their_addr, ipstr, INET6_ADDRSTRLEN) ;
				 
//		cout << " CReceiver::main() : recvfrom " << numbytes << "bytes"<< endl;
		if (numbytes%MMDAQ_INT32_SIZE) {
			cerr << "CReceiver::main(void*): bad packet size: numbytes%"<< MMDAQ_INT32_SIZE <<" != 0" << endl;
			continue;
		}
		
		m_recv_mutex->lock( thread_id(), thread_name(),"main(), 1" );
		size_t datasize = m_datacontainer->size();
		m_recv_mutex->unlock();

      bool got_end_of_event = false;
      if (numbytes == 4) {
         unsigned *ip = reinterpret_cast<unsigned*>(buf);
         if (*ip == SRS_NEXT_EVENT)
            got_end_of_event = true;
      }

      bool ignore_frame = m_pause_run;
      
      if (got_end_of_event) {
         ++m_received_events_counter;
//         std::cout << "CReceiver::main(void*) got_end_of_event event = " << m_received_events_counter << std::endl;

         m_pause_run = m_pause_run_requested || (datasize > 1500);
         if (ignore_frame) {
            ++m_ignored_events_counter;
         }
      }
      
      if (ignore_frame ) {
         continue;
      }
      
      CUDPData* udpframe = 0;
		try {
         //allocate mmeory for data, copy data there and push pointer into container - all in CUDPData
         udpframe = new CUDPData(buf, numbytes, ipaddr, &in_time, framecounter, m_received_events_counter);
      } catch (...) {
         std::cerr << "CReceiver::main(void*) : ERROR new CUDPData thrown an exception" << std::endl;
         continue;
      }
      
		/** LOCK RECEIVER DATA **/
		m_recv_mutex->lock( thread_id() , thread_name() ,"main(), 2");
		m_datacontainer->push_back(udpframe);
		m_internal_queue_size = m_datacontainer->size();
		m_event_ready |= got_end_of_event; // set to true if got event	
		m_recv_mutex->unlock( );
		/** UNLOCK RECEIVER DATA UNLOCKED **/
		
      //check full event then signal to decoder:
      if (got_end_of_event) {
//         std::cout << "CReceiver::main(void*) got_end_of_event m_datacontainer.size()=" << m_datacontainer->size()<< std::endl;
//			if (m_pause_run_requested) {
//				m_pause_run = true;
//			}
			signal_condition(m_action_event_received);
			//got_end_of_event = false;
      }
		++framecounter;
   } // while(1)

   return 0;
}

char * CReceiver::get_ip_str(const struct sockaddr *sa, char *s, socklen_t maxlen)
{
	switch(sa->sa_family) {
		case AF_INET:
			inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
						 s, maxlen);
			break;
			
		case AF_INET6:
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
						 s, maxlen);
			break;
			
		default:
			strncpy(s, "Unknown AF", maxlen);
			return NULL;
	}
	
	return s;
}

