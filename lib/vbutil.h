
// vbutil.h
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
// original version written by Dan Kimberg substantial code
// merged in from other members of team VoxBo

#ifndef VBUTIL_H
#define VBUTIL_H

#include <vector>
#include <list>
#include <set>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <zlib.h>
#include <limits.h>
#include <float.h>
#include <unistd.h> // For getopt().
#include <termios.h> // For definition of struct winsize.
#include <sys/ioctl.h> // For definition of struct winsize.
#include <cmath>
#include <stdint.h>
#include "tokenlist.h"
#include "genericexcep.h"
#include "vbversion.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

using namespace std;
using boost::format;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef STRINGLEN
#define STRINGLEN 16384
#endif

#ifndef vbforeach
#define vbforeach BOOST_FOREACH
#endif

// for convenience
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

enum VB_datatype {vb_byte,vb_short,vb_long,vb_float,vb_double,  \
                  vb_int32=vb_long,vb_int16=vb_short,vb_int8=vb_byte};
enum VB_byteorder{ENDIAN_LITTLE=0,ENDIAN_BIG=1};
enum VB_imagetype {vb_1d,vb_2d,vb_3d,vb_4d,vb_other};
enum VB_ERROR_LEVEL {VB_INFO, VB_WARNING, VB_ERROR, VB_EXCEPTION};

// flags for things to do with time series data
static const long MEANSCALE=    1<<0;
static const long DETREND=      1<<1;
static const long EXCLUDEERROR= 1<<2;
static const long AUTOCOR=      1<<3;

// some random constants
static const double PI=atan(1.0)*4.0;
static const double TWOPI=atan(1.0)*8.0;
static const double TINY=0.05;   // used in resampling

// ridiculous class for a block of memory (useful so that we can avoid
// pointers in other classes and therefore use automatic copy
// constructors)
class dblock {
public:
  dblock();
  dblock(uint8 *indata,uint32 insize);
  dblock(const dblock &src);
  dblock &operator=(const dblock &src);
  void init(uint8 *indata,uint32 insize);
  ~dblock();
  uint8 *data;
  uint32 size;
};

// ridiculous class for return codes
class vbreturn {
  int err;
  string msg;
public:
  vbreturn() {err=0; msg="";}
  vbreturn(int ee) {err=ee; msg=(err ? "bad" : "good");}
  vbreturn(int ee,string mm) {err=ee; msg=mm;}
  int error() {return err;}
  string message() {return msg;}
  operator int(){return err;}
};

class bitmask {
public:
  bitmask();
  bitmask(const bitmask &old);
  ~bitmask();
  void operator=(const bitmask &old);
  bool operator[](size_t pos) const;
  void resize (int bits);
  void clear();
  int count() const;
  void set(size_t pos);
  void unset(size_t pos);
  size_t size() const;
  unsigned char *mask;
  int bytes;
  size_t sz;
};

bool operator==(const bitmask &a,const bitmask &b);
bool operator<(const bitmask &a,const bitmask &b);

// zfile abstracts FILE and gzFile for writing.  this is helpful so we
// can open the file in one mode or the other, and then use a
// consistent API.  zlib makes this easy for reading, but not writing.
// zfile is also nice because we could eventually add bzip2 or other
// compression algorithms.

class zfile {
public:
  zfile();
  bool open(string fname,const char *mode,int8 compressed=-1);
  size_t write(const void *ptr,size_t bytes);
  int seek(off_t offset,int whence);
  off_t tell();
  void close();
  void close_and_unlink();
  operator bool() const;
private:
  string filename;
  bool zflag;
  FILE *fp;
  gzFile zfp;
};

// vglob abstracts glob a little

class vglob {
public:
  vglob();
  vglob(const string pat,uint32 flags=0);
  void load(const string pat,uint32 flags=0);
  void clear();
  bool empty();  // convenience
  void append(const string pat,uint32 flags=0);
  size_t size();
  enum {f_dirsonly=1,f_hidden=1<<1,f_filesonly=1<<2};
  operator tokenlist() const;
  string operator[](size_t ind);
  vector<string> names;
  // these four defs allow tokenlist to work with boost::foreach
  typedef vector<string>::iterator iterator;
  typedef vector<string>::const_iterator const_iterator;
  iterator begin();
  iterator end();
};

// jobid is useful as a unitary job identifier

class jobid {
public:
  jobid();
  jobid(int ss,int jj);
  int snum,jnum;
};

bool operator==(const jobid &j1,const jobid &j2);
bool operator<(const jobid &j1,const jobid &j2);

// tcolor -- a class for cycleable color mapping

class tcolor {
public:
  tcolor() {init();}
  int r,g,b;
  int index;
  static const int max=10;
  void init() {index=-1;next();}
  void next() {
    index++;
    if (index>max) index=0;
    int rs[]={255,  0,  0,210, 255,255, 26, 75,113,181, 40};
    int gs[]={  0,255,  0,210, 145,  0,184,140, 71,110,128};
    int bs[]={  0,  0,255,  0,   0,225, 63,204,204, 89, 36};
    r=rs[index];g=gs[index];b=bs[index];
  }
};

// vbrect -- a class for rectangles

class vbrect {
public:
  vbrect() {x=y=0; w=h=-1;}
  vbrect(int32 xx,int32 yy,int32 ww,int32 hh) {x=xx;y=yy;w=ww;h=hh;}
  //vbrect operator|(vbrect &r);   // union
  vbrect operator&(const vbrect &r);   // intersection
  operator bool() {return (w>-1 && h>-1);}
  void print();
  int32 x,y,w,h;
};

// util.cpp
void dbasename(const char *str,char *res);
void ddirname(const char *str,char *tmp);

string xfilename(const string &str);
string xdirname(const string &str,int times=1);
string xrootname(const string &str);
string xstripwhitespace(const string &str,const string whitespace="\t\n\r ");
string xsetextension(const string &str,const string newextension,bool multiflag=0);
string xgetextension(const string &str,bool multiflag=0);
string xcmdline(int argc,char **argv);
string xgetcwd();
string xabsolutepath(string path);

int createfullpath(const string &pathname);
ino_t vb_direxists(string dirname);
ino_t vb_fileexists(string dirname);
int rmdir_force(string dirname);
int touch(string fname);
void parentify(char *str,int n);
void lockfile(FILE *fp,short locktype,int pos=0,int len=1);
void unlockfile(FILE *fp,int pos=0,int len=1);
FILE *lockfiledir(char *fname);
void unlockfiledir(FILE *fp);
int32 ncores();
bool equali(const string &a,const string &b);
bool dancmp(const char *a,const char *b);
string vb_toupper(const string &str);
string vb_tolower(const string &str);
VB_datatype str2datatype(string str);

void guessorigin(int &d1,int &d2,int &d3);
int fill_vars2(string &str,map<string,string> vars);
int fill_vars(string &str,tokenlist &args);
map<string,string> envmap(char **env);
void replace_string(string &target,const string &s1,const string &s2);
string varname(const string &a);
string vb_getchar(const string &prompt);
int copyfile(string in,string out);
void GetElapsedTime(long start,long end,int &hrs,int &mins,int &secs);
vector<int32> numberlist(const string &str);
string textnumberlist(const vector<int>nums);
set<int32> numberset(const string &str);
string textnumberset(const set<int32>nums);
string VBRandom_nchars(int n);
uint32 VBRandom();
uint64 VBRandom64();
int32 strtol(const string &str);
double strtod(const string &str);
pair<bool,int32> strtolx(const string &str);
pair<bool,double> strtodx(const string &str);

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

long msecs(const struct timeval &tt);
struct timeval operator-(struct timeval tv1,struct timeval tv2);
struct timeval operator+(struct timeval tv1,struct timeval tv2);
struct timeval operator+(struct timeval tv1,int usec);
struct timeval operator+=(struct timeval &tv1,int usec);
bool operator<(struct timeval tv1,struct timeval tv2);
bool operator>(struct timeval tv1,struct timeval tv2);
bool operator<=(struct timeval tv1,struct timeval tv2);
bool operator>=(struct timeval tv1,struct timeval tv2);
void operator+=(vector<int> &vec,int n);

int morethan(const void *first,const void *second);
void stripchars(char *str,const char *chars);
void stripchars(string &str,const char *chars);
// long memsize_avcore();
// long memsize_core();
void maketimedate(string &time,string &date);
string timedate();
int appendline(string fname,string str);
string uniquename(string prefix);
string prettysize(uint32 size);
int getdatasize(VB_datatype tp);

// endian.cpp

void swapn(unsigned char *uc,int dsize,int len=1);
void swap(uint16 *sh,int len=1);
void swap(int16 *sh,int len=1);
void swap(uint32 *sh,int len=1);
void swap(int32 *sh,int len=1);
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

// convenience class for mapping two integer values onto something
class twovals {
public:
  twovals() {v1=0;v2=0;}
  twovals(int32 vv1,int32 vv2) {v1=vv1;v2=vv2;}
  int32 v1,v2;
};

bool operator<(const twovals &t1,const twovals &t2);

// vbreports.cpp
void vb_buildindex(string homedir);

/*********************************************************************
* Required constants.                                                *
*********************************************************************/
const int TIME_STR_LEN = 256;
const int OPT_STRING_LENGTH = 256;

/*********************************************************************
* This struct is used to assemble all the information about the      *
* options in char **argv.                                            *
*                                                                    *
* optName - the name of the option.                                  *
* requiredArg - true if the option needs an argument, else false.    *
* optArg - holds an option's argument as a C++ string object.        *
* typeArg - specifies the type of the option's argument. See the     *
*           comments in the implementation of processOpts() for the  *
*           allowed types.                                           *
* optPresent - true if the option is actually present in char **argv,*
*              i.e., if the program was called with the option.      *
*********************************************************************/
struct optInfo
{
  char optName;
  bool requiredArg;
  string optArg;
  char typeArg;
  bool optPresent;
}; // typedef struct optInfo

/*********************************************************************
* Function prototypes for printing VoxBo error messages.             *
*********************************************************************/
void printErrorMsg(const VB_ERROR_LEVEL level,string theMsg);
void printErrorMsg(const VB_ERROR_LEVEL level,string theMsg,
		   const unsigned int lineNo,const char *func,const char *file);
void printErrorMsgAndExit(const VB_ERROR_LEVEL level,string theMsg,
			  const unsigned int lineNo, const char *func, const char *file,
			  const unsigned short exitValue);
void printErrorMsgAndExit(const VB_ERROR_LEVEL level,string theMsg,const unsigned short exitValue);

int validateOrientation(const string s);
int interleavedorder(int ind,int total);

class miniarg {
 public:
  string flag;
  string flag2;
  int argcnt;
  int present;
  tokenlist args;
  void clear() {
    flag = "";
    flag2 = "";
    present=0;
    argcnt = 0;
    args.clear();
  }
};

class arghandler {
 public:
  arghandler(); 
  ~arghandler();
  int parseArgs(int argc, char **argv);
  tokenlist getFlaggedArgs(string flag);
  tokenlist getUnflaggedArgs(); 
  void setArgs(string flag, string flag1, int size); 
  string badArg();
  int flagPresent(string flag);
 private:
  int getSize(string flag);
  vector<miniarg>miniargs;
  // tokenlist members;
  tokenlist temp;
  tokenlist unflagged;
  int argnum;
  string errmsg;
};

#endif // VBUTIL_H
