/*
 *  CMMDaqException.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#ifndef CMMDaqException_h
#define CMMDaqException_h

#include <exception>
#include <string>

class CMMDaqException : public std::exception 
{
	std::string m_message;
	virtual const char* what() const throw()
	{
		return m_message.c_str();
	}
	
public:
	CMMDaqException() : m_message("") {};
	CMMDaqException(const std::string &message): m_message(message) {};
	CMMDaqException(const char *message): m_message(message) {};
	~CMMDaqException() throw() {;};
	
	const std::string &msg() const;
};

#endif

