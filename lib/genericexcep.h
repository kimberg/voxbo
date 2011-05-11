
// genericexcep.h
// 
// Copyright (c) 1998-2001 by The VoxBo Development Team

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
// original version written by Kosh Banerjee

using namespace std;

#ifndef GENERICEXCEP_H
#define GENERICEXCEP_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include "vbutil.h"

/*********************************************************************
* Required constants.                                                *
*********************************************************************/
const int INT_STRING_SIZE = 5;
const int EXCEPTION_MSG_STRING_LENGTH = 256;

/********************************************************************
* A generic exception class.                                        *
********************************************************************/
class GenericExcep: public exception
{
  private:
    int lineNo;
    string fileName;
    string funcName;
    string message;
    string errMsg;
		string caller;

    static string error;
    static string file;
    static string lineNumb;
    static string func;

		void init(const int line, const char *fileNm, const char *func);

  public:
    GenericExcep (const int lineNo, const char *fileName,
    const char *funcName, const char *mesg);

    GenericExcep (const int lineNo, const char *fileName,
    const char *funcName, const string mesg);

    // DYK: added the below to satisfy virtual requirement new with gcc 3.2 (?)
    ~GenericExcep() throw() { }

    virtual void what(int line, string file, string function);
    virtual void whatAbort(int line, string file, string function);
    virtual void whatNoExit(int line, string file, string function);
    virtual void what();
    
}; // GenericExcep

#endif // GENERICEXCEP_H
