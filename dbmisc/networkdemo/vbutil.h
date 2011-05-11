
// vbutil.h
// prototypes, etc. for use by voxbolib
// Copyright (c) 1998-2005 by The VoxBo Development Team

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
// substantial code merged in from Kosh Banerjee

using namespace std;

#ifndef VBUTIL_H
#define VBUTIL_H

#include <vector>
#include <list>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <regex.h>
#include <unistd.h> // For getopt().
#include <termios.h> // For definition of struct winsize.
#include <sys/ioctl.h> // For definition of struct winsize.
#include <cmath>
#include <glob.h>
#include "tokenlist.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef STRINGLEN
#define STRINGLEN 1024
#endif

typedef unsigned long ulong;

enum VB_datatype {vb_byte,vb_short,vb_long,vb_float,vb_double};
enum VB_byteorder{ENDIAN_LITTLE=0,ENDIAN_BIG=1};
enum VB_imagetype {vb_1d,vb_2d,vb_3d,vb_4d,vb_other};
//enum VB_loglevel {vb_never,vb_onerror,vb_always};

// flags for things to do with time series data
static const long MEANSCALE=    1<<0;
static const long DETREND=      1<<1;
static const long EXCLUDEERROR= 1<<2;

// some random constants
static const double PI=3.14159265358979;
static const double TINY=0.05;   // used in resampling

// util.cpp
void dbasename(const char *str,char *res);
void ddirname(const char *str,char *tmp);

string xfilename(const string &str);
string xdirname(const string &str);
string xrootname(const string &str);
string xstripwhitespace(const string &str,const string whitespace="\t\n ");
string xsetextension(const string &str,const string newextension);
string xgetextension(const string &str);

int createfullpath(const string &pathname);
ino_t vb_direxists(string dirname);
ino_t vb_fileexists(string dirname);
void deletesequence(int seqnum);
int touch(string fname);
void parentify(char *str,int n);
void lockfile(FILE *fp,short locktype,int pos=0,int len=1);
void unlockfile(FILE *fp,int pos=0,int len=1);
FILE *lockfiledir(char *fname);
void unlockfiledir(FILE *fp);
bool equali(const string &a,const string &b);
void glob_transfer(tokenlist &toks,glob_t &gb);
bool dancmp(const char *a,const char *b);
string vb_toupper(const string &str);
string vb_tolower(const string &str);
VB_datatype str2datatype(string str);

void guessorigin(int &d1,int &d2,int &d3);
int fill_vars2(string &str,tokenlist &vars);
int fill_vars2(string &str,map<string,string> &vars);
void fill_vars2(string &str,char **myenv);
int fill_vars(string &str,tokenlist &args);
void fill_vars(string &str,char **myenv);
void replace_string(string &target,const string &s1,const string &s2);
string varname(const string &a);
string vb_getchar(const string &prompt);
int copyfile(string in,string out);
void GetElapsedTime(long start,long end,int &hrs,int &mins,int &secs);
string textnumberlist(const vector<int>nums);
vector<int> numberlist(const string &str);
string VBRandom_filename();
unsigned long VBRandom();
long strtol(const string &str);
double strtod(const string &str);

template <class T>
string strnum(T d)
{
  char foo[STRINGLEN];
  sprintf(foo,"%ld",(long)d);
  return (string)foo;
}

string strnum(int d,int p);
string strnum(float d);
string strnum(double d);

struct timeval operator-(struct timeval &tv1,struct timeval &tv2);
struct timeval operator+(struct timeval &tv1,struct timeval &tv2);
bool operator<(struct timeval &tv1,struct timeval &tv2);
bool operator>(struct timeval &tv1,struct timeval &tv2);

void stripchars(char *str,const char *chars);
// long memsize_avcore();
// long memsize_core();
void maketimedate(string &time,string &date);
string timedate();
int appendline(string fname,string str);
string uniquename(string prefix);
string prettysize(long size);

// endian.cpp

void swapn(unsigned char *uc,int dsize,int len=1);
void swap2(unsigned char *uc,int len=1);
void swap(short *sh,int len=1);
void swap(unsigned short *sh,int len=1);
void swap(long *lng,int len=1);
void swap(unsigned long *lng,int len=1);
void swap(int *lng,int len=1);
void swap(unsigned int *lng,int len=1);
void swap(float *flt,int len=1);
void swap(double *dbl,int len=1);
VB_byteorder my_endian();

// connect.cpp
int safe_recv(int sock,char *buf,int len,float secs);
int safe_send(int sock,char *buf,int len,float secs);
int safe_connect(struct sockaddr_un *addr,float secs);
int safe_connect(struct sockaddr_in *addr,float secs);
int safe_connect(struct sockaddr *addr,float secs);
int send_file(int s,string fname);
int receive_file(int s,string fname,int filesize);

// vbreports.cpp
void vb_buildindex();


#endif // VBUTIL_H
