/*
 *  CListener.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#include "CListener.h"
#include "MBtools.h"
#include "CMMDaqException.h"

#include <cstring>
#include <iostream>
#include <string>
#include <arpa/inet.h>


using std::cout;
using std::cerr;
using std::endl;
using std::string;


CListener::CListener()
:m_fdmax(0), m_newfd(0)   

{
	struct addrinfo hints, *ai, *p;

	FD_ZERO(&m_master);    // clear the master and temp sets
	FD_ZERO(&m_read_fds);
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		string str = "CListener() selectserver: " + static_cast<string> ( gai_strerror(rv) );
		throw CMMDaqException(str);
		//exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
		m_listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (m_listener < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(m_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		if (bind(m_listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(m_listener);
			continue;
		}
		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		string str =  "CListener() selectserver: failed to bind" ;
		throw CMMDaqException(str);
		//exit(2);
	}
	
	freeaddrinfo(ai); // all done with this
	
	// listen
//	if (listen(m_listener, 10) == -1) {
//		perror("CListener() ");
//		throw CMMDaqException("aa");
//		//exit(3);
//	}
	
	// add the listener to the master set
	FD_SET(m_listener, &m_master);
	
	// keep track of the biggest file descriptor
	m_fdmax = m_listener; // so far, it's this one
}


CListener::~CListener()
{

}

// get sockaddr, IPv4 or IPv6:
void *CListener::get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void CListener::Run()
{
	
	// main loop
	for(;;) {
		m_read_fds = m_master; // copy it
		if (select(m_fdmax+1, &m_read_fds, NULL, NULL, NULL) == -1) {
			perror("CListener() select");
			throw CMMDaqException();
			//exit(4);
		}
		
		// run through the existing connections looking for data to read
		for(i = 0; i <= m_fdmax; i++) {
			if (FD_ISSET(i, &m_read_fds)) { // we got one!!
				if (i == m_listener) {
					int newfd;        // newly accept()ed socket descriptor
					struct sockaddr_storage remoteaddr; // client address
					socklen_t addrlen;
					
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(m_listener, (struct sockaddr *)&remoteaddr,	&addrlen);
					
					if (newfd == -1) {
						perror("CListener() accept");
					} else {
						FD_SET(newfd, &m_master); // add to master set
						if (newfd > m_fdmax) {    // keep track of the max
							m_fdmax = newfd;
						}
						cout << "CListener() selectserver: new connection from " 
						<< inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN) 
						<< "on socket " << newfd << endl;
								 
					}
				} else {
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
						// got error or connection closed by client
						if (nbytes == 0) {
							// connection closed
							printf("CListener() selectserver: socket %d hung up\n", i);
						} else {
							perror("CListener() recv");
						}
						close(i); // bye!
						FD_CLR(i, &m_master); // remove from master set
					} else {
						// we got some data from a client
						for(j = 0; j <= m_fdmax; j++) {
							// send to everyone!
							if (FD_ISSET(j, &m_master)) {
								// except the listener and ourselves
								if (j != m_listener && j != i) {
									if (send(j, buf, nbytes, 0) == -1) {
										perror("CListener() send");
									}
								}
							}
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END for(;;)--and you thought it would never end!
	
	
}
