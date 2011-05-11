
// tokenlist.h
// prototypes, etc. for use by voxbolib
// Copyright (c) 1998-1999 by The VoxBo Development Team

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
#include <cstring>

// argument parsing stuff

class tokenlist {
  deque<string> args;
  string separator,commentchars,tokenchars;
  string openquotechars,closequotechars;
  string blank;
  string fullstring;
  vector<int> poslist;
  int terminalquotes;  // if set, mid-token quotes terminate token when closed
  typedef deque<string>::iterator CI;
 public:
  tokenlist();
  tokenlist(const tokenlist &a);
  tokenlist(const char *);
  tokenlist(const string str);
  tokenlist(const string str,const string sep);
  void clear();
  int size() const;
  int ParseLine(const char *line);
  int ParseLine(const string str);
  int ParseFirstLine(const string filename);
  void SetSeparator(const string str);
  void SetCommentChars(const string str);
  void SetTokenChars(const string str);
  void SetQuoteChars(const string str);
  void TerminalQuotes(int flag);
  void print();
  string Tail(int num=1);
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
  int Truncate(int start, int end);
  void Sort(bool (*cmpfn)(string ,string ));
  tokenlist& operator+(const tokenlist &added);
  string operator[](const int index);
  const char *operator()(const int index) const;
  operator deque<string>&();
  string MakeString();
};

bool length_sorter(const string a,const string b);
bool alpha_sorter(const string a,const string b);

#endif // TOKENLIST_H

