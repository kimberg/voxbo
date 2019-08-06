
// genericexcep.cpp
//
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

#include "genericexcep.h"
#include "string.h"

/********************************************************************
 * The GenericExcep class:                                           *
 ********************************************************************/

/********************************************************************
 * Initializing the static data members.                             *
 ********************************************************************/
string GenericExcep::error = "[E] ";
string GenericExcep::file = " In File: [";
string GenericExcep::lineNumb = "] Line Number: [";
string GenericExcep::func = "] In function: [";

/********************************************************************
 * Constructors for GenericExcep:                                    *
 ********************************************************************/
GenericExcep::GenericExcep(const int line, const char *fileNm, const char *func,
                           const char *mesg) {
  this->init(line, fileNm, func);
  this->message = string(mesg);

}  // GenericExcep::GenericExcep(const int line, const char *fileNm, const char
   // *func, const char *mesg)

GenericExcep::GenericExcep(const int line, const char *fileNm, const char *func,
                           const string mesg) {
  this->init(line, fileNm, func);
  this->message = mesg;

}  // GenericExcep::GenericExcep(const int line, const char *fileNm, const char
   // *func, const string mesg)

void GenericExcep::init(const int line, const char *fileNm, const char *func) {
  this->lineNo = line;
  this->fileName = fileNm;
  this->funcName = func;
  this->errMsg = "";
  this->caller = "";
}  // GenericExcep::init(const int line, const char *fileNm, const char *func)

void GenericExcep::what(int line, string file, string function) {
  this->whatNoExit(line, file, function);
  exit(1);

}  // virtual void GenericExcep::what(int, string, string) const

void GenericExcep::whatAbort(int line, string file, string function) {
  this->whatNoExit(line, file, function);
  abort();
}  // virtual void GenericExcep::whatAbort(int line, string file, string
   // function)

void GenericExcep::whatNoExit(int line, string file, string function) {
  return;  // FIXME DYK: this quick and dirty fix eliminates all those
           // spurious console messages from vb_vector
  char intString[INT_STRING_SIZE];
  (void)memset(intString, 0, sizeof(intString));
  (void)sprintf(intString, "%d", this->lineNo);

  errMsg = this->error + this->message + this->file + this->fileName +
           this->lineNumb + intString + this->func + this->funcName + "]";

  (void)memset(intString, 0, sizeof(intString));
  (void)sprintf(intString, "%d", line);
  caller = "[X] FROM: Line Number [" + (string)intString + "] Function [" +
           function + "] File [" + file + "]";

  cerr << caller << endl;
  cerr << errMsg << endl;

}  // virtual void GenericExcep::whatNoExit(int, string, string) const

void GenericExcep::what() {
  this->what(0, "UNKNOWN", "UNKNOWN");
}  // virtual void GenericExcep::what() const
