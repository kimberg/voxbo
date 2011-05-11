
// vbscripttools.h
// 
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Mjumbe Poe

#ifndef VBSCRIPTTOOLS_H
#define VBSCRIPTTOOLS_H

#include <list>
#include <string>
#include <map>

#include "vbprefs.h"
#include <boost/foreach.hpp>

//#define VBDEBUG

using std::string;
using std::list;
using std::map;

namespace VB
{
	// scripting system object structures
	class Definitions;
	class ExecDef;
	class JobType;
	class BlockDef;
	
	class Sequence;
	struct Exec;
	struct Job;
	struct Block;
	
	class DataSet;
	
  class Exception
  {
      string _str;
    public:
      Exception(const string& s) : _str(s) {}
      string get_str() { return _str; }
  };
  
  extern const unsigned ID_LENGTH;
  extern const unsigned DS_LENGTH;
	extern const string WORKING_DIR_STRING;
	extern const string SEQUENCE_NAME_STRING;

  string& ltrim( string &str, const string &whitespace = "\t ");
  string& rtrim( string &str, const string &whitespace = "\t ");
  string& trim( string &str, const string &whitespace = "\t ");
  string zero_pad(unsigned n, unsigned length);
  list<string> split(const string& str, const string& del);
  bool matches(const string& str, const string& regexp);
  
  // ostream manipulators
  extern ostream& vb_nl(ostream&);
  extern ostream& vb_endl(ostream&);
  struct vb_indent
  {
    vb_indent(int = 2, char = ' ');
 		int indent_amount;
 		char indent_char;
  };
	extern ostream& operator<<(ostream&, vb_indent);

  // debug output function
  template <typename type>
	#ifdef VBDEBUG
	inline void DEBUG_OUT(const type& t)
	{
		cerr << t;
		cerr.flush();
	}
	#else
	inline void DEBUG_OUT(const type&)
	{
	}
	#endif
}

#endif
