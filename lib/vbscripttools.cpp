
// vbscripttools.cpp
//
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
// original version written by Mjumbe Poe

#include "vbscripttools.h"
#include "vbutil.h"

using namespace std;
using namespace VB;

const unsigned VB::ID_LENGTH = 6;
const unsigned VB::DS_LENGTH = 2;
const string VB::WORKING_DIR_STRING = "DIR";
const string VB::SEQUENCE_NAME_STRING = "NAME";

string& VB::ltrim(string& str, const string& whitespace) {
  size_t idx = str.find_first_not_of(whitespace);
  if (idx != string::npos)
    str = str.substr(idx);
  else
    str = "";

  return str;
}

string& VB::rtrim(string& str, const string& whitespace) {
  size_t idx = str.find_last_not_of(whitespace);
  if (idx != string::npos) str = str.substr(0, idx + 1);

  return str;
}

string& VB::trim(string& str, const string&) { return rtrim(ltrim(str)); }

string VB::zero_pad(unsigned n, unsigned length) {
  string s(length, '0');
  const char* d = "0123456789";

  for (int pos = length - 1; n > 0 && pos >= 0; --pos) {
    s[pos] = d[n % 10];
    n /= 10;
  }
  return s;
}

list<string> VB::split(const string& str, const string& del) {
  list<string> toks;
  size_t beg = 0, end;

  do {
    end = str.find(del, beg);
    toks.push_back(str.substr(beg, end - beg));
    beg = end + del.length();
  } while (end != string::npos);
  return toks;
}

bool VB::matches(const string& str, const string& regexp) {
  list<string> literals = split(regexp, "*");
  size_t beg = 0, pos;

  string last = literals.back();
  if (last.length() > 0 && last.length() <= str.length() &&
      str.substr(str.length() - last.length()) != last)
    return false;

  string first = literals.front();
  literals.pop_front();
  if (str.substr(0, first.length()) != first) return false;

  vbforeach(string & l, literals) {
    pos = str.find(l, beg);
    if (pos == string::npos) return false;
    beg = pos + l.length();
  }

  return true;
}

/* Indentation information map */
struct vb_indentation_info {
  char indent_char;
  int num_indent;
};

map<ostream*, vb_indentation_info> indent_map;

/* New line (and indent) */
ostream& VB::vb_nl(ostream& out) {
  out << "\n";

  // if the ostream is int the indent map then indent the next line as well.
  map<ostream*, vb_indentation_info>::iterator indent_iter =
      indent_map.find(&out);
  if (indent_iter != indent_map.end()) {
    vb_indentation_info& ii = indent_iter->second;
    out << string(ii.num_indent, ii.indent_char);
  }

  return out;
}

/* New line, flush (and indent) */
ostream& VB::vb_endl(ostream& out) {
  out << std::endl;

  // if the ostream is int the indent map then indent the next line as well.
  map<ostream*, vb_indentation_info>::iterator indent_iter =
      indent_map.find(&out);
  if (indent_iter != indent_map.end()) {
    vb_indentation_info& ii = indent_iter->second;
    out << string(ii.num_indent, ii.indent_char);
  }

  return out;
}

/* Indent */
vb_indent::vb_indent(int n, char c) : indent_amount(n), indent_char(c) {}

ostream& VB::operator<<(ostream& out, vb_indent indent) {
  // try to insert the ostream into the indent map
  // ::NOTE:: the following line should work.  bug in the stl implementation???
  //	pair<map<ostream*, vb_indentation_info>::iterator, bool>
  // iter_status_pair = indent_map.insert(const_cast<ostream*>(&out));

  pair<map<ostream*, vb_indentation_info>::iterator, bool> iter_status_pair =
      indent_map.insert(
          pair<ostream*, vb_indentation_info>(&out, vb_indentation_info()));
  map<ostream*, vb_indentation_info>::iterator indent_iter =
      iter_status_pair.first;
  vb_indentation_info& ii = indent_iter->second;

  // if the ostream was not in the map, then initialize indentation number
  if (iter_status_pair.second) {
    ii.num_indent = 0;
    ii.indent_char = indent.indent_char;
  }

  // set the indentation amount
  unsigned old_indent = ii.num_indent;
  ii.num_indent += indent.indent_amount;
  if (ii.num_indent < 0) {
    ii.num_indent = 0;
  }

  int amount_indented = ii.num_indent - old_indent;
  if (amount_indented >= 0) out << string(amount_indented, ii.indent_char);
  //	else
  //		out << string(-amount_indented, '\b');

  return out;
}
