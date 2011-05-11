
// voxbolib.cpp
// VoxBo library of misc functions
// Copyright (c) 1998-2003 by The VoxBo Development Team

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
// code contributed by Kosh Banerjee and Thomas King

using namespace std;

#include "vbutil.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <utime.h>
#include <ctype.h>
#include <pwd.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>
#include <iostream>
#include <fstream>

int
touch(string fname)
{
  return (utime(fname.c_str(),NULL));
}

string
xdirname(const string &str)
{
  string res(str);
  // remove trailing slashes
  while (res.size() && res[res.size()-1] == '/')
    res.erase(res.size()-1,1);
  // remove trailing non-slashes
  while (res.size() && res[res.size()-1] != '/')
    res.erase(res.size()-1,1);
  // remove trailing slashes
  while (res.size() && res[res.size()-1] == '/')
    res.erase(res.size()-1,1);
  if (!res.size())
    res=".";
  return res;
}

string
xfilename(const string &str)
{
  string res(str);
  // remove trailing slashes
  while (res.size() && res[res.size()-1] == '/')
    res.erase(res.size()-1,1);
  // chop off at last slash
  if (res.rfind("/") != string::npos)
    res.erase(0,res.rfind("/")+1);
  if (!res.size())
    res="/";
  return res;
}

string
xrootname(const string &str)
{
  string tmp=str;
  size_t pos=tmp.rfind(".");
  if (pos != string::npos)
    tmp.erase(pos,tmp.size()-pos);
  return tmp;
}

string
xstripwhitespace(const string &str,const string whitespace)
{
  string::size_type first,last;
  first=str.find_first_not_of(whitespace);
  last=str.find_last_not_of(whitespace);
  if (first != string::npos)
    return str.substr(first,last-first+1);
  else
    return "";
}

string
xsetextension(const string &str,const string newextension)
{
  size_t pos=str.rfind(".");
  size_t slashpos=str.rfind("/");
  if (slashpos!=string::npos && pos!=string::npos && slashpos>pos)
    pos=string::npos;
  string newname=str;
  if (!newextension.size()) {
    if (pos==string::npos)
      return newname;
    else {
      newname.erase(pos,str.size()-pos);
      return newname;
    }
  }
  if (pos==string::npos)
    return str+(string)"."+newextension;
  newname.replace(pos,str.size()-pos,(string)"."+newextension);
  return newname;
}

string
xgetextension(const string &str)
{
  size_t pos=str.rfind(".");
  if (pos==string::npos)
    return (string)"";
  pos++;
  return str.substr(pos,str.size()-pos);
}

void
parentify(char *fname,int n)
{
  char *slash;

  for(int i=0; i<n; i++) {
    if (strlen(fname) <= 0)
      return;
    fname[strlen(fname)-1]=0;
    slash = strrchr(fname,'/');
    if (slash) *(slash+1)=0;
    else return;
  }
}

int
createfullpath(const string &pathname)
{
  tokenlist trim,path;
  string fullpath;
  struct stat st;
  int err;

  trim.ParseLine(pathname);
  path.SetSeparator("/");
  path.ParseLine(trim[0]);
  if (trim[0][0]=='/')
    fullpath='/';
  for (int i=0; i<path.size(); i++) {
    fullpath+=path[i];
    err=stat(fullpath.c_str(),&st);
    if (err == -1 && errno == ENOENT) {
      if (mkdir(fullpath.c_str(),0xFFFF))
        return 100;
    }
    fullpath+='/';
  }
  return 0;  // no error!
}

ino_t
vb_direxists(string dirname)
{
  struct stat st;
  int err=stat(dirname.c_str(),&st);
  if (err)
    return 0;
  if (!(S_ISDIR(st.st_mode)))
    return 0;
  return st.st_ino;
}

ino_t
vb_fileexists(string dirname)
{
  struct stat st;
  int err=stat(dirname.c_str(),&st);
  if (err)
    return 0;
  if (!(S_ISREG(st.st_mode)))
    return 0;
  return st.st_ino;
}

FILE *
lockfiledir(char *fname)
{
  FILE *fp;
  char dname[STRINGLEN],lname[STRINGLEN];
  strcpy(dname,xdirname(fname).c_str());
  sprintf(lname,"%s/.lock",dname);
  fp = fopen(lname,"w");
  lockfile(fp,F_WRLCK);
  return fp;
}

void
unlockfiledir(FILE *fp)
{
  if (!fp)
    return;
  unlockfile(fp);
  fclose(fp);
}

void
lockfile(FILE *fp,short locktype,int pos,int len)
{
  struct flock numlock;
  numlock.l_type = locktype;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp),F_SETLKW,&numlock);
}

void
unlockfile(FILE *fp,int pos,int len)
{
  fflush(fp);
  struct flock numlock;
  numlock.l_type = F_UNLCK;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp),F_SETLKW,&numlock);
}

bool
equali(const string &a,const string &b)
{
  if (a.size() != b.size())
    return 0;
  for (int i=0; i<(int)a.size(); i++) {
    if (tolower(a[i]) != tolower(b[i]))
      return 0;
  }
  return 1;
}

void
glob_transfer(tokenlist &toks,glob_t &gb)
{
  toks.clear();
  for (int i=0; i<(int)gb.gl_pathc; i++)
    toks.Add(gb.gl_pathv[i]);
}

bool
dancmp(const char *a,const char *b)
{
  if (a == (char *)NULL || b == (char *)NULL)
    return 0;
  int al=strlen(a);
  int bl=strlen(b);
  if (al != bl)
    return 0;
  for (int i=0; i<al; i++) {
    if (a[i] != b[i])
      return 0;
  }
  return 1;
}

string
vb_toupper(const string &str)
{
  string tmp=str;
  for (int i=0; i<(int)str.size(); i++)
    tmp[i]=toupper(str[i]);
  return tmp;
}

string
vb_tolower(const string &str)
{
  string tmp=str;
  for (int i=0; i<(int)str.size(); i++)
    tmp[i]=tolower(str[i]);
  return tmp;
}

VB_datatype
str2datatype(string str)
{
  vb_tolower(str);
  if (str=="int16" || str=="integer" || str=="short")
    return vb_short;
  else if (str=="int32" || str=="long")
    return vb_long;
  else if (str=="float")
    return vb_float;
  else if (str=="double")
    return vb_double;
  else
    return vb_byte;
}

// guessorigin takes voxel dimensions and guesses an origin using a
// truly dumb heuristic.  the function is only here to make sure we
// use the same truly dumb heuristic wherever we use one.

void
guessorigin(int &d1,int &d2,int &d3)
{
  d1 = lround((float)d1 / 2.0);
  d2 = lround((float)d2 * 2.0 / 3.0);
  d3 = lround((float)d3 / 2.0);
}

void
GetElapsedTime(long start,long end,int &hrs,int &mins,int &secs)
{
  long elapsed;

  elapsed = end - start;
  if (elapsed < 0)
    elapsed = 0;
  hrs = mins = secs = 0;
  hrs = (int)(elapsed / 3600);
  elapsed -= 3600 * ((int)(elapsed/3600));
  mins = (int)(elapsed / 60);
  elapsed -= 60 * ((int)(elapsed/60));
  secs = elapsed;
}

int
fill_vars2(string &str,tokenlist &vars)
{
  size_t pos;
  tokenlist var;
  var.SetSeparator("=");
  var.SetQuoteChars("");
  int cnt=0;
  
  for (int i=0; i<vars.size(); i++) {
    var.ParseLine(vars[i]);
    if (var.size()<1)
      continue;
    string tag=(string)"$("+var[0]+")";
    while ((pos=str.find(tag)) != string::npos) {
      str.replace(pos,tag.size(),var.Tail());
      cnt++;
    }
  }
  return (cnt);
}

void
fill_vars2(string &str,char **myenv)
{
  int i=0;
  tokenlist envlist;

  while (myenv[i])
    envlist.Add(myenv[i++]);
  fill_vars2(str,envlist);
}

/* ::NOTE:: fill_vars2(string&,map&) added by mjumbe on 2007 Jul 12 */
int
fill_vars2(string &str,map<string,string> &mymap)
{
	size_t pos;
  int cnt=0;
  
  map<string,string>::iterator myiter;
  for (myiter = mymap.begin(); myiter != mymap.end(); ++myiter) {
    string tag = string("$(") + myiter->first + ")";
    while ((pos = str.find(tag)) != string::npos) {
      str.replace(pos,tag.size(),myiter->second);
      cnt++;
    }
  }
  return (cnt);
}

int
fill_vars(string &str,tokenlist &args)
{
  size_t pos;
  tokenlist newvars;
  tokenlist var;
  var.SetSeparator("=");
  int cnt=0,dupfound,i,j;
  string v0,v1;
  
  // FIXME BROKEN? first go through, sort, and eliminate duplicates
  while (0) {
  //   for (i=args.size()-1; i>=0; i--) {
    if (newvars.size()==0) {
      newvars.Add(args[i]);
      continue;
    }
    v0=args[i];
    dupfound=0;
    for (j=0; j<newvars.size(); j++) {
      if (varname(args[i])==varname(newvars[j]))
        dupfound=1;
    }
    if (dupfound)
      continue;
    if (args[i].size())
      newvars.AddFront(args[i]);
  }
  newvars=args;

  // FIXME -- the following loops infinitely if VAR=xxxVARxxx
  // start at the end, so that later additions supercede older vars
  for (i=newvars.size()-1; i>=0; i--) {
    var.ParseLine(newvars[i]);
    if (var.size()<1)
      continue;
    while ((pos=str.find((string)"$"+var[0])) != string::npos) {
      str.replace(pos,var[0].size()+1,var.Tail());
      cnt++;
    }
  }
  return (cnt);
}

// varname() returns the variable name in an env var (aaa=foo returns "aaa")

string
varname(const string &a)
{
  tokenlist aa;
  aa.SetSeparator("=");
  aa.ParseLine(a);
  return aa[0];
}

// overloaded wrapper for the above

void
fill_vars(string &str,char **myenv)
{
  // char **ee=myenv;
  int i=0;
  tokenlist envlist;

  while (myenv[i])
    envlist.Add(myenv[i++]);
  fill_vars(str,envlist);
}

void
replace_string(string &target,const string &s1,const string &s2)
{
  size_t pos;
  while ((pos=target.find(s1)) != string::npos)
    target.replace(pos,s1.size(),s2);
}

string
vb_getchar(const string &prompt)
{
  termios tsave,tnew;
  tcgetattr(0,&tsave);
  tcgetattr(0,&tnew);
  tnew.c_lflag&=~(ICANON|ECHO);
  tcsetattr(0,TCSADRAIN,&tnew);
  string str;
  cout << prompt << flush;
  str=cin.get();
  tcsetattr(0,TCSADRAIN,&tsave);
  return str;
}

int
copyfile(string infile,string outfile)
{
  const int BUFSIZE=4096;
  ifstream inf;
  ofstream outf;
  char buf[BUFSIZE];
  struct stat sti,sto;
  int err,erri,erro;

  erri=stat(infile.c_str(),&sti);
  erro=stat(outfile.c_str(),&sto);
  if (erri)
    return 101;
  if (!erro) {
    if (sti.st_dev == sto.st_dev && sti.st_ino == sto.st_ino)
      return 0;  // no error!
  }
  inf.open(infile.c_str());
  if (!inf)
    return 102;
  outf.open(outfile.c_str());
  if (!outf) {
    inf.close();
    return 103;
  }
  while (inf.good() && outf.good()) {
    inf.read(buf,BUFSIZE);
    outf.write(buf,inf.gcount());
  }
  err=0;
  if (!inf.eof())
    err=104;
  if (!outf.good())
    err=105;
  inf.close();
  outf.close();
  return err;  // no error!
}

string
textnumberlist(const vector<int>nums)
{
  string ret;
  if (!(nums.size()))
    return "";
  int rangestart=nums[0];
  int rangeend=nums[0];
  for (int i=1; i<(int)nums.size(); i++) {
    if (nums[i]-rangeend !=1) {
      if (ret.size())ret+=",";
      if (rangeend==rangestart)
        ret+=strnum(rangestart);
      else
        ret+=strnum(rangestart)+"-"+strnum(rangeend);
      rangestart=rangeend=nums[i];
    }
    else
      rangeend=nums[i];
  }
  if (ret.size())ret+=",";
  if (rangeend==rangestart)
    ret+=strnum(rangestart);
  else
    ret+=strnum(rangestart)+"-"+strnum(rangeend);
  return ret;
}

vector<int>
numberlist(const string &str)
{
  vector<int> numbers;
  int thisnum,endpoint;
  tokenlist ranges;

  ranges.SetTokenChars(",-:");
  ranges.ParseLine(str);
  for (int i=0; i<ranges.size(); i++) {
    if (isdigit(ranges[i][0])) {
      thisnum=strtol(ranges[i]);
      numbers.push_back(thisnum);
    }
    if (dancmp(ranges(i+1),"-")) {
      if (isdigit(ranges[i+2][0])) {
  endpoint=strtol(ranges[i+2]);
  for (int j=thisnum+1; j<= endpoint; j++)
    numbers.push_back(j);
  i+=2;
      }
    }
  }
  return numbers;
}

string
VBRandom_filename()
{
  string lookup="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  string foo;
  unsigned long r;
  for (int i=0; i<2; i++) {
    r=VBRandom();
    for (int i=0; i<6; i++) {
      foo+=lookup[r&0x1F];
      r=r>>5;
    }
  }    
  return foo;
}

unsigned long
VBRandom()
{
  struct stat st;
  int fd;
  unsigned long buf;

  if (stat("/dev/urandom",&st))
    return 0;
  fd=open("/dev/urandom",O_RDONLY);
  if (fd == -1)
    return 0;
  read(fd,&buf,sizeof(unsigned long));
  close(fd);
  return buf;
}

long
strtol(const string &str)
{
  return strtol(str.c_str(),NULL,10);
}

double
strtod(const string &str)
{
  return (strtod(str.c_str(),NULL));
}

string
strnum(int d,int p)
{
  char foo[STRINGLEN];
  char format[STRINGLEN];
  sprintf(format,"%%0%dd",p);
  sprintf(foo,format,d);
  string tmp=foo;
  return tmp;
}

string
strnum(float d)
{
  char foo[STRINGLEN];
  sprintf(foo,"%g",d);
  string tmp=foo;
  return tmp;
}

string
strnum(double d)
{
  char foo[STRINGLEN];
  sprintf(foo,"%g",d);
  string tmp=foo;
  return tmp;
}

void
stripchars(char *str,const char *chars)
{
  for (int i=0; i<(int)strlen(str); i++)
    if (strchr(chars,str[i]))
      str[i]='\0';
}

// long
// memsize_core()
// {
//   return(sysconf(_SC_PAGESIZE)*sysconf(_SC_PHYS_PAGES)/(1024*1024));
// }

// long
// memsize_avcore()
// {
//   return(sysconf(_SC_PAGESIZE)*sysconf(_SC_AVPHYS_PAGES)/(1024*1024));
// }

string
timedate()
{
  string tstr,dstr;
  maketimedate(tstr,dstr);
  return (dstr+(string)"_"+tstr);
}

void
maketimedate(string &t,string &d)
{
  char timestr[STRINGLEN],datestr[STRINGLEN];
  struct tm *mytm;
  time_t mytime;

  tzset();                     // make sure all times are timezone corrected
  mytime = time(NULL);
  mytm = localtime(&mytime);
  strftime(timestr,STRINGLEN,"%H:%M:%S",mytm);
  strftime(datestr,STRINGLEN,"%Y_%m_%d",mytm);
  t=timestr;
  d=datestr;
}

int
appendline(string fname,string str)
{
  FILE *fp = fopen(fname.c_str(),"a");
  if (!fp)
    return(101);
  fprintf(fp,"%s\n",xstripwhitespace(str).c_str());
  fclose(fp);
  return (0);  // no error!
}

string
uniquename(string prefix)
{
  static int index=100;
  if (prefix.size()==0) {
    char hname[STRINGLEN];
    if (gethostname(hname,STRINGLEN-1))
      strcpy(hname,"nohost");
    hname[STRINGLEN-1]='\0';
    prefix=hname;
  }
  string name=prefix+"_"+strnum(getpid())+"_"+strnum(index);
  index++;
  return name;
}

string
prettysize(long size)
{
  string ret="";
  char ps[STRINGLEN];
  sprintf(ps,"%ld",size);
  float dsize=(float)size/1024.0;
  if (dsize>1024) {
    dsize/=1024.0;
    sprintf(ps,"%.1fMB",dsize);
  }
  if (dsize>1024) {
    dsize/=1024.0;
    sprintf(ps,"%.1fGB",dsize);
  }
  if (dsize>1024) {
    dsize/=1024.0;
    sprintf(ps,"%.1fTB",dsize);
  }
  
  return (string)ps;
}

struct timeval
operator-(struct timeval &tv1,struct timeval &tv2)
{
  struct timeval ret;
  ret.tv_sec=tv1.tv_sec-tv2.tv_sec;
  ret.tv_usec=tv1.tv_usec-tv2.tv_usec;
  if (ret.tv_usec<0) {
    ret.tv_sec--;
    ret.tv_usec+=1000000;
  }
  return ret;
}

struct timeval
operator+(struct timeval &tv1,struct timeval &tv2)
{
  timeval ret;
  ret.tv_sec=tv1.tv_sec+tv2.tv_sec;
  ret.tv_usec=tv1.tv_usec+tv2.tv_usec;
  if (ret.tv_usec>1000000) {
    ret.tv_sec++;
    ret.tv_usec-=1000000;
  }
  return ret;
}

bool
operator<(struct timeval &tv1,struct timeval &tv2)
{
  if (tv1.tv_sec<tv2.tv_sec)
    return 1;
  if (tv1.tv_sec==tv2.tv_sec && tv1.tv_usec<tv2.tv_usec)
    return 1;
  return 0;
}

bool
operator>(struct timeval &tv1,struct timeval &tv2)
{
  if (tv1.tv_sec>tv2.tv_sec)
    return 1;
  if (tv1.tv_sec==tv2.tv_sec && tv1.tv_usec>tv2.tv_usec)
    return 1;
  return 0;
}

