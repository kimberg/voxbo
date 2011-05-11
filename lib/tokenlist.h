
// tokenlist.h
// prototypes, etc. for use by voxbolib
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
// original version written by Dan Kimberg

using namespace std;

#ifndef TOKENLIST_H
#define TOKENLIST_H

#include <stdio.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <string>
#include <stdint.h>

// for convenience
typedef uint32_t uint32;

// argument parsing stuff

class tokenlist {
private:
  deque<string> args;
  string separator,commentchars,tokenchars;
  string openquotechars,closequotechars;
  string blank;
  string fullstring;
  vector<size_t> poslist;
  int terminalquotes;  // if set, mid-token quotes terminate token when closed
 public:
  tokenlist();
  tokenlist(const tokenlist &a);
  tokenlist(const char *);
  tokenlist(const string str);
  tokenlist(const string str,const string sep);
  void clear();
  size_t size() const;
  int ParseLine(const char *line);
  int ParseLine(const string str);
  int ParseFirstLine(const string filename);
  int ParseFile(const string filename,const string commentchars="");
  void SetSeparator(const string str);
  void SetCommentChars(const string str);
  void SetTokenChars(const string str);
  void SetQuoteChars(const string str);
  void TerminalQuotes(uint32 flag);
  void print();
  string Tail(size_t num=1);
  int Transfer(int argc,char *argv[]);
  void Add(const char *str);
  void Add(const string &str);
  void AddBack(const char *str);
  void AddBack(const string &str);
  void AddFront(const char *str);
  void AddFront(const string &str);
  void DeleteFirst();
  void DeleteLast();
  int Remove(int start, int end);
  // int Truncate(int start, int end);
  void Sort(bool (*cmpfn)(string ,string ));
  tokenlist operator+(const tokenlist &added);
  tokenlist operator+(const string tok);
  tokenlist operator+(const char *tok);
  tokenlist& operator+=(const tokenlist &added);
  tokenlist& operator+=(const string tok);
  tokenlist& operator+=(const char *tok);
  string &operator[](const int index);
  const char *operator()(const int index) const;
  operator deque<string>();
  operator vector<string>();
  string MakeString(int num=0);
  // these four defs allow tokenlist to work with boost::foreach
  typedef deque<string>::iterator iterator;
  typedef deque<string>::const_iterator const_iterator;
  iterator begin();
  iterator end();
};

bool length_sorter(const string a,const string b);
bool alpha_sorter(const string a,const string b);

#endif // TOKENLIST_H

