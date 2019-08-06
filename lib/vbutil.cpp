
// vbutil.cpp
// VoxBo library of misc functions
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
// code contributed by Kosh Banerjee and Thomas King

#include "vbutil.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <math.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <fstream>
#include <iostream>

using namespace std;
using boost::format;
using boost::thread;

int touch(string fname) { return (utime(fname.c_str(), NULL)); }

string xdirname(const string &str, int times) {
  string res(str);
  for (int i = 0; i < times; i++) {
    // remove trailing slashes
    while (res.size() && res[res.size() - 1] == '/')
      res.erase(res.size() - 1, 1);
    // remove trailing non-slashes
    while (res.size() && res[res.size() - 1] != '/')
      res.erase(res.size() - 1, 1);
    // remove trailing slashes
    while (res.size() && res[res.size() - 1] == '/')
      res.erase(res.size() - 1, 1);
    if (!res.size()) res = ".";
  }
  return res;
}

string xfilename(const string &str) {
  string res(str);
  if (res.empty()) return res;
  // remove trailing slashes
  while (res.size() && res[res.size() - 1] == '/') res.erase(res.size() - 1, 1);
  // chop off at last slash
  if (res.rfind("/") != string::npos) res.erase(0, res.rfind("/") + 1);
  if (!res.size()) res = "/";
  return res;
}

string xrootname(const string &str) {
  string tmp = str;
  size_t pos = tmp.rfind(".");
  if (pos != string::npos) tmp.erase(pos, tmp.size() - pos);
  return tmp;
}

string xstripwhitespace(const string &str, const string whitespace) {
  string::size_type first, last;
  first = str.find_first_not_of(whitespace);
  last = str.find_last_not_of(whitespace);
  if (first != string::npos)
    return str.substr(first, last - first + 1);
  else
    return "";
}

string xsetextension(const string &str, const string newextension,
                     bool multiflag) {
  size_t pos, slashpos;
  if (!multiflag) {
    slashpos = str.rfind("/");
    pos = str.rfind(".");
    // if last . is before a slash, then there's no extension
    if (slashpos != string::npos && pos != string::npos && slashpos > pos)
      pos = string::npos;
  } else {
    slashpos = str.rfind("/");
    if (slashpos == string::npos)
      pos = str.find(".");
    else
      pos = str.find(".", slashpos);
  }
  string newname = str;
  // if no new extension, strip old one including .
  if (!newextension.size()) {
    if (pos == string::npos)
      return newname;
    else {
      newname.erase(pos, str.size() - pos);
      return newname;
    }
  }
  if (pos == string::npos) return str + (string) "." + newextension;
  newname.replace(pos, str.size() - pos, (string) "." + newextension);
  return newname;
}

string xgetcwd() {
  char buf[PATH_MAX * 2];
  buf[PATH_MAX * 2 - 1] = 0;
  if (getcwd(buf, PATH_MAX * 2 - 1)) return (string)buf;
  return ".";
}

string xabsolutepath(string path) {
  string tmp = xstripwhitespace(path);
  if (!tmp.size()) return tmp;
  if (tmp[0] == '/' || tmp[0] == '~') return tmp;
  return xgetcwd() + "/" + tmp;
}

string xgetextension(const string &str, bool multiflag) {
  size_t pos, slashpos;
  if (!multiflag) {
    slashpos = str.rfind("/");
    pos = str.rfind(".");
    // if last . is before a slash, then there's no extension
    if (slashpos != string::npos && pos != string::npos && slashpos > pos)
      pos = string::npos;
  } else {
    slashpos = str.rfind("/");
    if (slashpos == string::npos)
      pos = str.find(".");
    else
      pos = str.find(".", slashpos);
  }
  if (pos == string::npos) return (string) "";
  pos++;
  return str.substr(pos, str.size() - pos);
}

string xcmdline(int argc, char **argv) {
  string ret = argv[0];
  for (int i = 1; i < argc; i++) ret += (string) " " + argv[i];
  return ret;
}

void parentify(char *fname, int n) {
  char *slash;

  for (int i = 0; i < n; i++) {
    if (strlen(fname) <= 0) return;
    fname[strlen(fname) - 1] = 0;
    slash = strrchr(fname, '/');
    if (slash)
      *(slash + 1) = 0;
    else
      return;
  }
}

int createfullpath(const string &pathname) {
  tokenlist trim, path;
  string fullpath;
  struct stat st;
  int err;

  trim.ParseLine(pathname);
  path.SetSeparator("/");
  path.ParseLine(trim[0]);
  if (trim[0][0] == '/') fullpath = '/';
  for (size_t i = 0; i < path.size(); i++) {
    fullpath += path[i];
    err = stat(fullpath.c_str(), &st);
    if (err == -1 && errno == ENOENT) {
      if (mkdir(fullpath.c_str(), 0755)) return 100;
    }
    fullpath += '/';
  }
  return 0;  // no error!
}

ino_t vb_direxists(string dirname) {
  struct stat st;
  int err = stat(dirname.c_str(), &st);
  if (err) return 0;
  if (!(S_ISDIR(st.st_mode))) return 0;
  return st.st_ino;
}

ino_t vb_fileexists(string dirname) {
  struct stat st;
  int err = stat(dirname.c_str(), &st);
  if (err) return 0;
  if (!(S_ISREG(st.st_mode))) return 0;
  return st.st_ino;
}

int rmdir_force(string dirname) {
  if (dirname == "") return 104;
  vglob vg(dirname + "/*");
  for (size_t i = 0; i < vg.size(); i++) unlink(vg[i].c_str());
  if (rmdir(dirname.c_str())) return (103);
  return (0);  // no error!
}

FILE *lockfiledir(char *fname) {
  FILE *fp;
  char dname[STRINGLEN], lname[STRINGLEN];
  strcpy(dname, xdirname(fname).c_str());
  sprintf(lname, "%s/.lock", dname);
  fp = fopen(lname, "w");
  lockfile(fp, F_WRLCK);
  return fp;
}

void unlockfiledir(FILE *fp) {
  if (!fp) return;
  unlockfile(fp);
  fclose(fp);
}

void lockfile(FILE *fp, short locktype, int pos, int len) {
  struct flock numlock;
  numlock.l_type = locktype;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp), F_SETLKW, &numlock);
}

void unlockfile(FILE *fp, int pos, int len) {
  fflush(fp);
  struct flock numlock;
  numlock.l_type = F_UNLCK;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp), F_SETLKW, &numlock);
}

int32 ncores() {
  // FIXME win32 issue
  int n = sysconf(_SC_NPROCESSORS_ONLN);
  if (n < 1) n = 1;
  return n;
}

bool equali(const string &a, const string &b) {
  if (a.size() != b.size()) return 0;
  for (size_t i = 0; i < a.size(); i++) {
    if (tolower(a[i]) != tolower(b[i])) return 0;
  }
  return 1;
}

bool dancmp(const char *a, const char *b) {
  if (a == (char *)NULL || b == (char *)NULL) return 0;
  int al = strlen(a);
  int bl = strlen(b);
  if (al != bl) return 0;
  for (int i = 0; i < al; i++) {
    if (a[i] != b[i]) return 0;
  }
  return 1;
}

int getdatasize(VB_datatype tp) {
  if (tp == vb_byte) return 1;
  if (tp == vb_short) return 2;
  if (tp == vb_long) return 4;
  if (tp == vb_float) return 4;
  if (tp == vb_double) return 8;
  return 0;
}

string vb_toupper(const string &str) {
  string tmp = str;
  for (size_t i = 0; i < str.size(); i++) tmp[i] = toupper(str[i]);
  return tmp;
}

string vb_tolower(const string &str) {
  string tmp = str;
  for (size_t i = 0; i < str.size(); i++) tmp[i] = tolower(str[i]);
  return tmp;
}

VB_datatype str2datatype(string str) {
  vb_tolower(str);
  if (str == "int16" || str == "integer" || str == "short")
    return vb_short;
  else if (str == "int32" || str == "long")
    return vb_long;
  else if (str == "float")
    return vb_float;
  else if (str == "double")
    return vb_double;
  else
    return vb_byte;
}

// guessorigin takes voxel dimensions and guesses an origin using (1)
// typical origins for MNI volumes; and (2) a truly dumb heuristic.

void guessorigin(int &d1, int &d2, int &d3) {
  d1 = lround((float)d1 / 2.0);
  d2 = lround((float)d2 * 2.0 / 3.0);
  d3 = lround((float)d3 / 2.0);
}

void GetElapsedTime(long start, long end, int &hrs, int &mins, int &secs) {
  long elapsed;

  elapsed = end - start;
  if (elapsed < 0) elapsed = 0;
  hrs = mins = secs = 0;
  hrs = (int)(elapsed / 3600);
  elapsed -= 3600 * ((int)(elapsed / 3600));
  mins = (int)(elapsed / 60);
  elapsed -= 60 * ((int)(elapsed / 60));
  secs = elapsed;
}

// fill_vars2() handles replacement of $(foo) --
// fill_vars2(string&,map&) added by mjumbe, dan blew away the
// original versions later

int fill_vars2(string &str, map<string, string> mymap) {
  size_t pos;
  int cnt = 0;

  map<string, string>::iterator myiter;
  for (myiter = mymap.begin(); myiter != mymap.end(); ++myiter) {
    string tag = string("$(") + myiter->first + ")";
    while ((pos = str.find(tag)) != string::npos) {
      str.replace(pos, tag.size(), myiter->second);
      cnt++;
    }
  }
  return (cnt);
}

// fill_vars handles replacement of $foo (no delimiters around foo)

int fill_vars(string &str, tokenlist &args) {
  size_t pos;
  tokenlist newvars;
  tokenlist var;
  var.SetSeparator("=");
  int cnt = 0, dupfound, i;
  string v0, v1;

  // FIXME BROKEN? first go through, sort, and eliminate duplicates
  while (0) {
    //   for (i=args.size()-1; i>=0; i--) {
    if (newvars.size() == 0) {
      newvars.Add(args[i]);
      continue;
    }
    v0 = args[i];
    dupfound = 0;
    for (size_t j = 0; j < newvars.size(); j++) {
      if (varname(args[i]) == varname(newvars[j])) dupfound = 1;
    }
    if (dupfound) continue;
    if (args[i].size()) newvars.AddFront(args[i]);
  }
  newvars = args;

  // FIXME -- the following loops infinitely if VAR=xxxVARxxx
  // start at the end, so that later additions supercede older vars
  for (i = newvars.size() - 1; i >= 0; i--) {
    var.ParseLine(newvars[i]);
    if (var.size() < 1) continue;
    while ((pos = str.find((string) "$" + var[0])) != string::npos) {
      str.replace(pos, var[0].size() + 1, var.Tail());
      cnt++;
    }
  }
  return (cnt);
}

map<string, string> envmap(char **env) {
  tokenlist t;
  map<string, string> mymap;
  t.SetSeparator("=");
  int index = 0;
  char *myenv = env[index];
  while (myenv) {
    t.ParseLine(myenv);
    mymap[t[0]] = t.Tail();
    myenv = env[++index];
  }
  return mymap;
}

// varname() returns the variable name in an env var (aaa=foo returns "aaa")

string varname(const string &a) {
  tokenlist aa;
  aa.SetSeparator("=");
  aa.ParseLine(a);
  return aa[0];
}

void replace_string(string &target, const string &s1, const string &s2) {
  size_t pos, minpos = 0;
  while ((pos = target.find(s1, minpos)) != string::npos) {
    target.replace(pos, s1.size(), s2);
    minpos = pos + s2.size();
  }
}

bool wildcard_compare(char *pTameText, char *pWildText,
                      bool bCaseSensitive = FALSE, char cAltTerminator = '\0') {
  bool bMatch = TRUE;
  char *pAfterLastWild =
      NULL;  // The location after the last '*', if we've encountered one
  char *pAfterLastTame = NULL;  // The location in the tame string, from which
                                // we started after last wildcard
  char t, w;

  // Walk the text strings one character at a time.
  while (1) {
    t = *pTameText;
    w = *pWildText;

    // How do you match a unique text string?
    if (!t || t == cAltTerminator) {
      // Easy: unique up on it!
      if (!w || w == cAltTerminator) {
        break;  // "x" matches "x"
      } else if (w == '*') {
        pWildText++;
        continue;  // "x*" matches "x" or "xy"
      } else if (pAfterLastTame) {
        if (!(*pAfterLastTame) || *pAfterLastTame == cAltTerminator) {
          bMatch = FALSE;
          break;
        }
        pTameText = pAfterLastTame++;
        pWildText = pAfterLastWild;
        continue;
      }
      bMatch = FALSE;
      break;  // "x" doesn't match "xy"
    } else {
      if (!bCaseSensitive) {  // Lowercase the characters to be compared.
        if (t >= 'A' && t <= 'Z') {
          t += ('a' - 'A');
        }
        if (w >= 'A' && w <= 'Z') {
          w += ('a' - 'A');
        }
      }

      // How do you match a tame text string?
      if (t != w) {  // The tame way: unique up on it!
        if (w == '*') {
          pAfterLastWild = ++pWildText;
          pAfterLastTame = pTameText;
          w = *pWildText;

          if (!w || w == cAltTerminator) {
            break;  // "*" matches "x"
          }
          continue;  // "*y" matches "xy"
        } else if (pAfterLastWild) {
          if (pAfterLastWild != pWildText) {
            pWildText = pAfterLastWild;
            w = *pWildText;
            if (!bCaseSensitive && w >= 'A' && w <= 'Z') {
              w += ('a' - 'A');
            }

            if (t == w) {
              pWildText++;
            }
          }
          pTameText++;
          continue;  // "*sip*" matches "mississippi"
        } else {
          bMatch = FALSE;
          break;  // "x" doesn't match "y"
        }
      }
    }

    pTameText++;
    pWildText++;
  }
  return bMatch;
}

string vb_getchar(const string &prompt) {
  termios tsave, tnew;
  tcgetattr(0, &tsave);
  tcgetattr(0, &tnew);
  tnew.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSADRAIN, &tnew);
  string str;
  cout << prompt << flush;
  str = cin.get();
  tcsetattr(0, TCSADRAIN, &tsave);
  return str;
}

int copyfile(string infile, string outfile) {
  const int BUFSIZE = 4096;
  ifstream inf;
  ofstream outf;
  char buf[BUFSIZE];
  struct stat sti, sto;
  int err, erri, erro;

  erri = stat(infile.c_str(), &sti);
  erro = stat(outfile.c_str(), &sto);
  if (erri) return 101;
  if (!erro) {
    if (sti.st_dev == sto.st_dev && sti.st_ino == sto.st_ino)
      return 0;  // no error!
  }
  inf.open(infile.c_str());
  if (!inf) return 102;
  outf.open(outfile.c_str());
  if (!outf) {
    inf.close();
    return 103;
  }
  while (inf.good() && outf.good()) {
    inf.read(buf, BUFSIZE);
    outf.write(buf, inf.gcount());
  }
  err = 0;
  if (!inf.eof()) err = 104;
  if (!outf.good()) err = 105;
  inf.close();
  outf.close();
  return err;  // no error!
}

// textnumberlist() takes a vector if ints and tries to represent it
// compactly as text.  so 1,2,6,7,8,9,11 would become 1-2,6-9,11

string textnumberlist(const vector<int32> nums) {
  string ret;
  if (!(nums.size())) return "";
  int rangestart = nums[0];
  int rangeend = nums[0];
  for (size_t i = 1; i < nums.size(); i++) {
    if (nums[i] - rangeend != 1) {
      if (ret.size()) ret += ",";
      if (rangeend == rangestart)
        ret += strnum(rangestart);
      else
        ret += strnum(rangestart) + "-" + strnum(rangeend);
      rangestart = rangeend = nums[i];
    } else
      rangeend = nums[i];
  }
  if (ret.size()) ret += ",";
  if (rangeend == rangestart)
    ret += strnum(rangestart);
  else
    ret += strnum(rangestart) + "-" + strnum(rangeend);
  return ret;
}

vector<int32> numberlist(const string &str) {
  vector<int32> numbers, empty;
  tokenlist ranges;
  pair<bool, int32> thisnum, endpoint;

  ranges.SetTokenChars(",-:");
  ranges.ParseLine(str);
  for (size_t i = 0; i < ranges.size(); i++) {
    if (isdigit(ranges[i][0])) {
      thisnum = strtolx(ranges[i]);
      if (thisnum.first) return empty;
      numbers.push_back(thisnum.second);
    }
    if (dancmp(ranges(i + 1), "-")) {
      if (isdigit(ranges[i + 2][0])) {
        endpoint = strtolx(ranges[i + 2]);
        if (endpoint.first) return empty;
        // the below para expands 94567-8 to 94567-94568
        if (endpoint.second < thisnum.second) {
          int dec = 10;
          while (thisnum.second / dec) {
            if (endpoint.second / dec == 0) {
              endpoint.second += (thisnum.second / dec) * dec;
              break;
            } else
              dec *= 10;
          }
        }
        for (int j = thisnum.second + 1; j <= endpoint.second; j++)
          numbers.push_back(j);
        i += 2;
      }
    }
  }
  return numbers;
}

set<int32> numberset(const string &str) {
  vector<int32> tmp;
  set<int32> ret;
  tmp = numberlist(str);
  for (size_t i = 0; i < tmp.size(); i++) ret.insert(tmp[i]);
  return ret;
}

string textnumberset(const set<int32> nums) {
  string ret;
  if (!(nums.size())) return "";
  int rangestart = *(nums.begin());
  int rangeend = rangestart;
  for (set<int>::iterator n = ++nums.begin(); n != nums.end(); n++) {
    if (*n - rangeend != 1) {
      if (ret.size()) ret += ",";
      if (rangeend == rangestart)
        ret += strnum(rangestart);
      else
        ret += strnum(rangestart) + "-" + strnum(rangeend);
      rangestart = rangeend = *n;
    } else
      rangeend = *n;
  }
  if (ret.size()) ret += ",";
  if (rangeend == rangestart)
    ret += strnum(rangestart);
  else
    ret += strnum(rangestart) + "-" + strnum(rangeend);
  return ret;
}

string VBRandom_nchars(int n) {
  string lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  string foo;
  uint32 r = 9999;
  int counter = 0;
  for (int i = 0; i < n; i++) {
    if (counter < 1) {
      r = VBRandom();
      counter = 6;
    }
    foo += lookup[r & 0x1F];
    r = r >> 5;
    counter--;
  }
  return foo;
}

uint32 VBRandom() {
  struct stat st;
  int fd;
  uint32 buf;

  if (stat("/dev/urandom", &st)) return 0;
  fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) return 0;
  read(fd, &buf, sizeof(uint32));
  close(fd);
  return buf;
}

uint64 VBRandom64() {
  struct stat st;
  int fd;
  uint32 buf;

  if (stat("/dev/urandom", &st)) return 0;
  fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) return 0;
  read(fd, &buf, sizeof(uint64));
  close(fd);
  return buf;
}

int32 strtol(const string &str) {
  string tmp = xstripwhitespace(str);
  stringstream ss(tmp);
  int32 res;
  ss >> res;
  if (ss.fail() || (size_t)ss.tellg() < tmp.size()) return 0;
  return res;
}

double strtod(const string &str) {
  string tmp = xstripwhitespace(str);
  stringstream ss(tmp);
  double res;
  ss >> res;
  if (ss.fail() || (size_t)ss.tellg() < tmp.size()) return 0;
  return res;
}

pair<bool, int32> strtolx(const string &str) {
  string tmp = xstripwhitespace(str);
  stringstream ss(tmp);
  int32 res;
  ss >> res;
  if (ss.fail() || (size_t)ss.tellg() < tmp.size())
    return pair<bool, int32>(1, 0);
  return pair<bool, int32>(0, res);
}

pair<bool, double> strtodx(const string &str) {
  string tmp = xstripwhitespace(str);
  stringstream ss(tmp);
  double res;
  ss >> res;
  if (ss.fail() || (size_t)ss.tellg() < tmp.size())
    return pair<bool, double>(1, 0.0);
  return pair<bool, double>(0, res);
}

string strnum(int d, int p) {
  string fmt = (format("%%0%dd") % p).str();
  return (format(fmt) % d).str();
}

string strnum(float d) { return (format("%g") % d).str(); }

string strnum(double d) { return (format("%g") % d).str(); }

void stripchars(char *str, const char *chars) {
  for (size_t i = 0; i < strlen(str); i++)
    if (strchr(chars, str[i])) {
      str[i] = '\0';
      break;
    }
}

void stripchars(string &str, const char *chars) {
  string::size_type first;
  first = str.find_first_of(chars);
  if (first != string::npos) str = str.substr(0, first);
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

string timedate() {
  string tstr, dstr;
  maketimedate(tstr, dstr);
  return (dstr + (string) "_" + tstr);
}

void maketimedate(string &t, string &d) {
  char timestr[STRINGLEN], datestr[STRINGLEN];
  struct tm *mytm;
  time_t mytime;

  tzset();  // make sure all times are timezone corrected
  mytime = time(NULL);
  mytm = localtime(&mytime);
  strftime(timestr, STRINGLEN, "%H:%M:%S", mytm);
  strftime(datestr, STRINGLEN, "%Y_%m_%d", mytm);
  t = timestr;
  d = datestr;
}

int appendline(string fname, string str) {
  FILE *fp = fopen(fname.c_str(), "a");
  if (!fp) return (101);
  if (fprintf(fp, "%s\n", xstripwhitespace(str).c_str()) < 0) {
    fclose(fp);
    return 102;
  }
  fclose(fp);
  return (0);  // no error!
}

string uniquename(string prefix) {
  static int index = 100;
  if (prefix.size() == 0) {
    char hname[STRINGLEN];
    if (gethostname(hname, STRINGLEN - 1)) strcpy(hname, "nohost");
    hname[STRINGLEN - 1] = '\0';
    prefix = hname;
  }
  string name = prefix + "_" + strnum(getpid()) + "_" + strnum(index);
  index++;
  return name;
}

string prettysize(uint32 size) {
  string ret = "";
  string ps;
  ps = (format("%d") % size).str();
  float dsize = (float)size / 1024.0;
  if (dsize > 1024) {
    dsize /= 1024.0;
    ps = (format("%.1fMB") % dsize).str();
  }
  if (dsize > 1024) {
    dsize /= 1024.0;
    ps = (format("%.1fGB") % dsize).str();
  }
  if (dsize > 1024) {
    dsize /= 1024.0;
    ps = (format("%.1fTB") % dsize).str();
  }

  return (string)ps;
}

long msecs(const struct timeval &tt) {
  return (long)tt.tv_sec * 1000 + (long)tt.tv_usec / 1000;
}

struct timeval operator-(struct timeval tv1, struct timeval tv2) {
  struct timeval ret;
  ret.tv_sec = tv1.tv_sec - tv2.tv_sec;
  ret.tv_usec = tv1.tv_usec - tv2.tv_usec;
  while (ret.tv_usec < 0) {
    ret.tv_sec--;
    ret.tv_usec += 1000000;
  }
  return ret;
}

struct timeval operator+(struct timeval tv1, struct timeval tv2) {
  timeval ret;
  ret.tv_sec = tv1.tv_sec + tv2.tv_sec;
  ret.tv_usec = tv1.tv_usec + tv2.tv_usec;
  while (ret.tv_usec > 1000000) {
    ret.tv_sec++;
    ret.tv_usec -= 1000000;
  }
  return ret;
}

struct timeval operator+(struct timeval tv1, int usec) {
  timeval ret;
  ret.tv_sec = tv1.tv_sec;
  ret.tv_usec = tv1.tv_usec + usec;
  while (ret.tv_usec > 1000000) {
    ret.tv_sec++;
    ret.tv_usec -= 1000000;
  }
  return ret;
}

struct timeval operator+=(struct timeval &tv1, int usec) {
  tv1.tv_usec += usec;
  while (tv1.tv_usec > 1000000) {
    tv1.tv_sec++;
    tv1.tv_usec -= 1000000;
  }
  return tv1;
}

bool operator<(struct timeval tv1, struct timeval tv2) {
  if (tv1.tv_sec < tv2.tv_sec) return 1;
  if (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec < tv2.tv_usec) return 1;
  return 0;
}

bool operator>(struct timeval tv1, struct timeval tv2) {
  if (tv1.tv_sec > tv2.tv_sec) return 1;
  if (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec > tv2.tv_usec) return 1;
  return 0;
}

bool operator<=(struct timeval tv1, struct timeval tv2) {
  if (tv1.tv_sec < tv2.tv_sec) return 1;
  if (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec <= tv2.tv_usec) return 1;
  return 0;
}

bool operator>=(struct timeval tv1, struct timeval tv2) {
  if (tv1.tv_sec > tv2.tv_sec) return 1;
  if (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec >= tv2.tv_usec) return 1;
  return 0;
}

// dumb little operator for incrementing lists of integers all at
// once.  FIXME used???

void operator+=(vector<int> &vec, int n) {
  for (size_t i = 0; i < vec.size(); i++) vec[i] += n;
}

// qsort comparison function

int morethan(const void *first, const void *second) {
  return (*(double *)first > *(double *)second);
}

dblock::dblock() {
  data = (uint8 *)NULL;
  size = 0;
}

dblock::dblock(uint8 *indata, uint32 insize) {
  data = (uint8 *)NULL;
  size = 0;
  init(indata, insize);
}

void dblock::init(uint8 *indata, uint32 insize) {
  if (data) delete[] data;
  data = new uint8[size];
  size = insize;
  memcpy(data, indata, size);
}

dblock::dblock(const dblock &src) { *this = src; }

dblock::~dblock() { delete[] data; }

dblock &dblock::operator=(const dblock &src) {
  if (!data || size != src.size) {
    size = src.size;
    if (data) delete[] data;
    data = new uint8[size];
  }
  memcpy(data, src.data, size);
  return *this;
}

bool operator<(const twovals &t1, const twovals &t2) {
  if (t1.v1 < t2.v1)
    return 1;
  else if (t1.v1 > t2.v1)
    return 0;
  if (t1.v2 < t2.v2) return 1;
  return 0;
}

void printErrorMsg(const VB_ERROR_LEVEL level, string theMsg) {
  string msg;
  switch (level) {
    case VB_INFO:
      printf("[I] %s\n", theMsg.c_str());
      break;
    case VB_WARNING:
      printf("[W] %s\n", theMsg.c_str());
      break;
    case VB_ERROR:
      printf("[E] %s\n", theMsg.c_str());
      break;
    case VB_EXCEPTION:
      printf("[X] %s\n", theMsg.c_str());
      break;
  }
}

void printErrorMsgAndExit(const VB_ERROR_LEVEL level, string theMsg,
                          const unsigned short exitValue) {
  printErrorMsg(level, theMsg);
  exit(exitValue);
}

void printErrorMsg(const VB_ERROR_LEVEL level, string theMsg,
                   const unsigned int lineNo, const char *func,
                   const char *file) {
  ostringstream theMsg2;
  theMsg2 << "LINE [" << lineNo << "] FUNCTION [" << func << "] FILE [" << file
          << "] " << theMsg;
  printErrorMsg(level, theMsg2.str());
}

void printErrorMsgAndExit(const VB_ERROR_LEVEL level, string theMsg,
                          const unsigned int lineNo, const char *func,
                          const char *file, const unsigned short exitValue) {
  printErrorMsg(level, theMsg, lineNo, func, file);
  exit(exitValue);
}

// This function checks if the passed orientation is well formed.
// If well formed, it returns 0. Otherwise it returns -1.

int validateOrientation(const string s) {
  if ((s.find("R") != string::npos || s.find("L") != string::npos) &&
      (s.find("A") != string::npos || s.find("P") != string::npos) &&
      (s.find("I") != string::npos || s.find("S") != string::npos) &&
      (s.size() == 3))
    return 0;
  else
    return -1;
}

// DYK: interleavedorder() returns the order in which a slice would
// have been acquired, for a given total number of slices.

int interleavedorder(int ind, int total) {
  if (total & 1) {  // ODD
    if (ind & 1)
      return (total / 2) + 1 + (ind / 2);
    else
      return (ind / 2);
  } else {  // EVEN
    if (ind & 1)
      return (ind / 2);
    else
      return (total / 2) + (ind / 2);
  }
}

arghandler::arghandler() {}
arghandler::~arghandler() {}

int arghandler::parseArgs(int argc, char **argv) {
  errmsg = "";
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      unflagged.Add(argv[i]);
      continue;
    }
    int foundit = 0;
    for (size_t j = 0; j < miniargs.size(); j++) {
      if (miniargs[j].flag == argv[i] || miniargs[j].flag2 == argv[i]) {
        if ((i + miniargs[j].argcnt) < argc) {
          for (int k = 0; k < miniargs[j].argcnt; k++)
            miniargs[j].args.Add(argv[i + k + 1]);
          miniargs[j].present = 1;
          i += miniargs[j].argcnt;
          foundit = 1;
        } else {
          errmsg = "bad argument structure -- not enough args for " +
                   (string)argv[i];
        }
      }
    }
    if (!foundit) errmsg = "bad argument structure -- flag " + (string)argv[i];
  }
  return 0;
}

tokenlist arghandler::getFlaggedArgs(string flag) {
  for (size_t i = 0; i < miniargs.size(); i++) {
    if (miniargs[i].flag == flag) return miniargs[i].args;
  }
  tokenlist tmp;
  return tmp;
}

tokenlist arghandler::getUnflaggedArgs() { return unflagged; }

void arghandler::setArgs(string flag, string flag1, int size) {
  miniarg newArg;
  newArg.clear();
  newArg.flag = flag;
  newArg.flag2 = flag1;
  newArg.argcnt = size;
  miniargs.push_back(newArg);
}

int arghandler::getSize(string flag) {
  for (size_t i = 0; i < miniargs.size(); i++)
    if (flag == miniargs[i].flag || flag == miniargs[i].flag2)
      return miniargs[i].argcnt;
  return 0;
}

string arghandler::badArg() { return errmsg; }

int arghandler::flagPresent(string flag) {
  for (size_t i = 0; i < miniargs.size(); i++)
    if (miniargs[i].flag == flag || miniargs[i].flag2 == flag)
      if (miniargs[i].present) return 1;
  return 0;
}

// bitmask

bitmask::bitmask() {
  mask = (unsigned char *)NULL;
  sz = 0;
  bytes = 0;
}

bitmask::bitmask(const bitmask &old) {
  mask = (unsigned char *)NULL;
  bytes = 0;
  sz = 0;
  *this = old;
}

void bitmask::operator=(const bitmask &old) {
  if (mask) free(mask);
  bytes = old.bytes;
  sz = old.sz;
  if (old.bytes) {
    mask = (unsigned char *)calloc(old.bytes, 1);
    memcpy(mask, old.mask, bytes);
  }
}

void bitmask::resize(int bits) {
  if (mask) free(mask);
  bytes = bits / 8;
  if (bits % 8) bytes++;
  sz = bits;
  mask = (unsigned char *)calloc(bytes, 1);
}

void bitmask::clear() {
  for (int i = 0; i < bytes; i++) mask[i] = 0;
}

bitmask::~bitmask() {
  if (mask) free(mask);
}

void bitmask::set(size_t pos) {
  int nbyte = pos / 8;
  int nbit = pos % 8;
  if (nbyte > bytes - 1) return;
  mask[nbyte] |= (1 << nbit);
}

void bitmask::unset(size_t pos) {
  int nbyte = pos / 8;
  int nbit = pos % 8;
  if (nbyte > bytes - 1) return;
  mask[nbyte] &= ~(0x01 << nbit);
}

bool bitmask::operator[](size_t pos) const {
  if (pos + 1 > sz) return 0;
  int nbyte = pos / 8;
  int nbit = pos % 8;
  if (nbyte > bytes - 1) return 0;
  return mask[nbyte] & (1 << nbit);
}

size_t bitmask::size() const { return sz; }

int bitmask::count() const {
  int total = 0;
  for (int i = 0; i < bytes; i++) {
    int cnt = 0;
    unsigned char num = mask[i];
    for (; num; num &= num - 1) cnt++;
    total += cnt;
  }
  return total;
}

bool operator==(const bitmask &a, const bitmask &b) {
  if (a.bytes != b.bytes) return 0;
  if (a.mask == NULL || b.mask == NULL) return 0;
  if (memcmp(a.mask, b.mask, a.bytes) == 0) return 1;
  return 0;
}

bool operator<(const bitmask &a, const bitmask &b) {
  if (a.bytes != b.bytes) return 0;
  if (a.mask == NULL || b.mask == NULL) return 0;
  if (memcmp(a.mask, b.mask, a.bytes) < 0) return 1;
  return 0;
}

// zfile

zfile::zfile() {
  fp = NULL;
  zfp = NULL;
  zflag = 0;
}

bool zfile::open(string fname, const char *mode, int8 compressed) {
  filename = fname;
  if (compressed == -1) {
    if (xgetextension(fname) == "gz")
      compressed = 1;
    else
      compressed = 0;
  }
  if (compressed) {
    zfp = gzopen(fname.c_str(), mode);
    zflag = 1;
    return (bool)zfp;
  } else {
    fp = fopen(fname.c_str(), mode);
    zflag = 0;
    return (bool)fp;
  }
}

size_t zfile::write(const void *ptr, size_t bytes) {
  if (zflag)
    return gzwrite(zfp, ptr, bytes);
  else
    return fwrite(ptr, 1, bytes, fp);
}

int zfile::seek(off_t offset, int whence) {
  if (zflag)
    return gzseek(zfp, offset, whence);
  else
    return fseek(fp, offset, whence);
}

off_t zfile::tell() {
  if (zflag)
    return gztell(zfp);
  else
    return ftell(fp);
}

void zfile::close() {
  if (zflag) {
    gzclose(zfp);
    zfp = NULL;
  } else {
    fclose(fp);
    fp = NULL;
  }
}

void zfile::close_and_unlink() {
  if (zflag) {
    gzclose(zfp);
    zfp = NULL;
  } else {
    fclose(fp);
    fp = NULL;
  }
  unlink(filename.c_str());
  filename = "";
}

zfile::operator bool() const { return ((zflag && zfp) || (!zflag && fp)); }

// vglob

vglob::vglob() {}

vglob::vglob(const string pat, uint32 flags) { load(pat, flags); }

void vglob::load(const string pat, uint32 flags) {
  clear();
  append(pat, flags);
}

void vglob::clear() { names.clear(); }

bool vglob::empty() { return (size() == 0); }

vglob::iterator vglob::begin() { return names.begin(); }

vglob::iterator vglob::end() { return names.end(); }

void vglob::append(const string pat, uint32 flags) {
  glob_t gb;
  glob(pat.c_str(), 0, NULL, &gb);
  for (size_t i = 0; i < gb.gl_pathc; i++) {
    if (flags) {
      struct stat st;
      int err = stat(gb.gl_pathv[i], &st);
      if (err)
        continue;  // if we can't stat it, it doesn't definitely satisfy the
                   // flags
      if (flags & f_dirsonly && !S_ISDIR(st.st_mode)) continue;
      if (flags & f_filesonly && !S_ISREG(st.st_mode)) continue;
    }
    names.push_back(gb.gl_pathv[i]);
  }
  globfree(&gb);
}

size_t vglob::size() { return names.size(); }

vglob::operator tokenlist() const {
  tokenlist tt;
  vbforeach(string s, names) tt.Add(s);
  return tt;
}

string vglob::operator[](size_t ind) { return names[ind]; }

// jobid

jobid::jobid() { snum = jnum = 0; }

jobid::jobid(int ss, int jj) {
  snum = ss;
  jnum = jj;
}

bool operator==(const jobid &j1, const jobid &j2) {
  if (j1.snum != j2.snum || j1.jnum != j2.jnum) return 0;
  return 1;
}

bool operator<(const jobid &j1, const jobid &j2) {
  if (j1.snum < j2.snum || (j1.snum == j2.snum && j1.jnum < j2.jnum)) return 1;
  return 0;
}

// vbrect methods

vbrect vbrect::operator&(const vbrect &rr) {
  int32 l = x;
  int32 r = x + w - 1;
  int32 t = y;
  int32 b = y + h - 1;
  if (rr.x > l) l = rr.x;
  if (rr.x + rr.w - 1 < r) r = rr.x + rr.w - 1;
  if (rr.y > t) t = rr.y;
  if (rr.y + rr.h - 1 < b) b = rr.y + rr.h - 1;
  return vbrect(l, t, r - l + 1, b - t + 1);
}

void vbrect::print() {
  cout << format("x=%d y=%d w=%d h=%d r=%d b=%d\n") % x % y % w % h %
              (x + w - 1) % (y + h - 1);
}
