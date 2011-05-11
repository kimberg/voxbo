
// dbserver.cpp
// DBserver member functions
// Copyright (c) 2007-2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Dongbo Hu

using namespace std;

#include <cstring>
#include "dbserver.h"

// set env dir 
void DBserver::setEnvHome(const string& inputStr)
{
  dbs.dirname = inputStr; 
}

// get env dir 
string DBserver::getEnvHome() 
{
 return dbs.dirname; 
}

// Defined for some utility functions that need env to build transaction handle 
DbEnv& DBserver::getEnv() 
{ 
  return dbs.env.getEnv(); 
} 

// Get error message (may be used for log file)
string DBserver::getErrMsg() 
{ 
  return errMsg; 
}
