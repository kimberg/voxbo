
// tokenlist.cpp
// class for tokenization
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

#include <algorithm>
#include <iostream>
#include <fstream>

#include "tokenlist.h"

tokenlist::tokenlist()
{
  SetSeparator(" \t\n\r");
  SetCommentChars("#");
  SetTokenChars("");
  SetQuoteChars("\"'");
  clear();
  blank="";
  terminalquotes=1;
}

// the following two functions are a stupid way of adding c++ string
// class support.  should really just use auto-cast of char ptrs.

tokenlist::tokenlist(const string str)
{
  SetSeparator(" \t\n\r");
  clear();
  ParseLine(str.c_str());
}

tokenlist::tokenlist(const string str,const string sep)
{
  SetSeparator(sep);
  clear();
  ParseLine(str.c_str());
}

int
tokenlist::ParseLine(const string str)
{
  return ParseLine(str.c_str());
}

int
tokenlist::ParseLine(const char *line)
{
  int i,count;
  size_t pos;

  clear();
  fullstring=line;
  poslist.clear();  // list of all token start positions
  i=count=0;
  while (line[i]) {
    string tmp;
    // skip leading whitespace (or whatever separators we've set)
    while (line[i] && separator.find(line[i]) != string::npos)
      i++;
    // if we're done we're done
    if (line[i]=='\0')
      break;
    // if our character is a token character, grab it and we're done
    if (tokenchars.find(line[i]) != string::npos) {
      poslist.push_back(i);
      tmp += line[i++];
    }
    // otherwise read to the next separator
    else {
      poslist.push_back(i);
      while ((line[i] != '\0') && separator.find(line[i]) == string::npos &&
	     tokenchars.find(line[i]) == string::npos) {
        if ((pos=openquotechars.find(line[i])) != string::npos) {
          i++;
          while ((line[i] != '\0') && (line[i]!=closequotechars[pos]))
            tmp += line[i++];
          if (line[i]==closequotechars[pos]) i++;   // skip over terminating quote
          if (terminalquotes)
            break;
        }
        else
          tmp += line[i++];
      }
    }
    if (commentchars.find(tmp[0]) != string::npos)
      break;
    else {
      args.push_back(tmp);
      count++;
    }
  }
  return count;
}

int
tokenlist::Transfer(int argc,char *argv[])
{
  int i,count;
  string tmp;

  clear();
  count=0;
  for (i=0; i<argc; i++) {
    tmp=argv[i];
    args.push_back(tmp);
    count++;
  }
  return count;
}

void
tokenlist::AddBack(const char *str)
{
  string tmp=str;
  args.push_back(tmp);
}

void
tokenlist::AddBack(const string &str)
{
  args.push_back(str);
}

void
tokenlist::Add(const char *str)
{
  AddBack(str);       // i.e., default to back
}

void
tokenlist::Add(const string &str)
{
  AddBack(str.c_str());       // i.e., default to back
}

void
tokenlist::AddFront(const char *str)
{
  string tmp=str;
  args.push_front(tmp);
}

void
tokenlist::AddFront(const string &str)
{
  args.push_front(str);
}

void
tokenlist::DeleteFirst()
{
  args.pop_front();
}

void
tokenlist::DeleteLast()
{
  args.pop_back();
}

int
tokenlist::Truncate(int start, int end) 
{
   if (end == -1) end = args.size();
   args.erase(args.begin(), args.begin() + start);
   args.erase(args.begin() + end, args.end());
   return 0;
}

int
tokenlist::Remove(int start, int end) {
    if (end == -1) end = args.size();
    args.erase(args.begin() + start, args.begin() + end); 
    return 0;
}

// grab the first line of a file, and parse that

int
tokenlist::ParseFirstLine(const string filename)
{
  const int BUFLEN=1024;
  ifstream fs;
  char buf[BUFLEN];

  fs.open(filename.c_str(),ios::in);
  if (!fs)
    return 0;
  fs.getline(buf,BUFLEN,'\n');
  fs.close();
  if (strlen(buf) == 0)
    return 0;

  return ParseLine(buf);
}

void
tokenlist::SetSeparator(const string str)
{
  separator=str;
}

void
tokenlist::TerminalQuotes(int flag)
{
  terminalquotes=flag;
}

void
tokenlist::SetCommentChars(const string str)
{
  commentchars=str;
}

void
tokenlist::SetTokenChars(const string str)
{
  tokenchars=str;
}

void
tokenlist::SetQuoteChars(const string quotechars)
{
  openquotechars=quotechars;
  closequotechars=openquotechars;
  char close;
  for (int i=0; i<(int)closequotechars.size(); i++) {
    close=quotechars[i];
    if (close=='[') closequotechars[i]=']';
    else if (close=='(') closequotechars[i]=')';
    else if (close=='{') closequotechars[i]='}';
  }
}

void
tokenlist::clear()
{
  args.clear();
  poslist.clear();
  fullstring="";
}

int
tokenlist::size() const
{
  return args.size();
}

void
tokenlist::print()
{
  if (args.size() == 0)
    return;
  int cnt=0;
  for (CI c=args.begin(); c!= args.end(); c++)
    printf("token %2d [%d chars]: %s\n",cnt++,(int)c->size(),c->c_str());
}

string
tokenlist::MakeString()
{
  string s;
  if (args.size() == 0)
    return (string)0;
  for (CI c=args.begin(); c!= args.end(); c++) {
    s+=c->c_str();
  if ((c+1) != args.end())
    s+=" ";
  }
  return s;
}

tokenlist::tokenlist(const tokenlist &a)
{
  *this = a;
}

tokenlist &
tokenlist::operator+(const tokenlist &added)
{
  for (int i=0; i<(int)added.args.size(); i++)
    Add(added.args[i]);
  return *this;
}

string
tokenlist::operator[](const int index)
{
  if (index > (int)args.size() - 1 || index < 0) {
    return blank;
  }
  else
    return (args[index]);
}

const char *
tokenlist::operator()(const int index) const
{
  if (index > (int)args.size() - 1 || index < 0)
    return blank.c_str();
  else
    return (args[index].c_str());
}

tokenlist::operator deque<string>&()
{
  return args;
}

string
tokenlist::Tail(int num)
{
  string ret=fullstring;
  if (num==(int)poslist.size())
    return "";
  if (num>0 && num<(int)poslist.size()) {
    int pos=poslist[num];
    if (pos>0 && pos<(int)fullstring.size())
      ret=fullstring.substr(pos);
  }
  size_t last=ret.find_last_not_of(separator);
  if (last!=string::npos)
    ret.erase(last+1);
  return ret;
}

void
tokenlist::Sort(bool (*cmpfn)(string,string))
{
  sort(args.begin(),args.end(),cmpfn);
}

bool
length_sorter(const string a,const string b)
{
  if (a.size()<b.size())
    return 1;
  return 0;
}

bool
alpha_sorter(const string a,const string b)
{
  if (a<b)
    return 1;
  return 0;
}
