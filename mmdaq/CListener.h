/*
 *  CListener.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#ifndef CListener_h
#define CListener_h


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "4950"   // port we're listening on


class CListener
{
	fd_set m_master;		// master file descriptor list
	fd_set m_read_fds;	// temp file descriptor list for select()
	int    m_fdmax;				// maximum file descriptor number
	
	int m_listener;     // listening socket descriptor
	int m_newfd;        // newly accept()ed socket descriptor
	
	struct sockaddr_storage m_remoteaddr; // client address
	socklen_t m_addrlen;
	
	char buf[256];    // buffer for client data
	size_t nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	
	int yes;        // for setsockopt() SO_REUSEADDR, below
	int i, j, rv;
	
	void * get_in_addr(struct sockaddr *sa);


public:
	CListener();
	~CListener();
	
	void Run();
	
};

#endif

