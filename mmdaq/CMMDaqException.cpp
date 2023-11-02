/*
 *  CMMDaqException.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#include "CMMDaqException.h"

using std::string;

const string& CMMDaqException::msg() const {
	return m_message;
}
