//
//  ProgException.h
//  
//
//  Created by Marcin Byszewski on 7/18/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//

#ifndef mmDAQ2_ProgException_h
#define mmDAQ2_ProgException_h


#include <string>
#include <ostream>


   

class CProgException {
   std::string m_msg;
   
public:
   CProgException(const std::string& msg) : m_msg(msg) {}
   friend std::ostream& operator<< (std::ostream &out, CProgException &pex) { out << pex.m_msg; return out;}
   
};



#endif
