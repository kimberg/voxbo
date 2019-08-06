
// vbprefs.h
// class defs, prototypes, etc. for prefs and host stuff
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

#ifndef VBPREFS_H
#define VBPREFS_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include "vbio.h"
#include "vbjobspec.h"

class VBPrefs;
class VBHost;
class VBResource;
class VBReservation;
class VBJobType;
class VBJobSpec;

typedef map<string, VBResource>::iterator RI;

// reasons a job can't run on a host
enum {
  NOTPROVIDED = 1,
  NOTRESPONDING = 2,
  NORESOURCE = 3,
  NOCPU = 4,
  NOPRI = 5,
  NOTYPE = 6,
  HOSTNOTAVAILABLE = 7
};

class VBHost {
 private:
 public:
  map<string, VBResource> resources;
  vector<VBReservation> reservations;
  vector<VBJobSpec> runninglist;
  float loadaverage;
  int ncpus[168];  // # cpus available for each hour of the week
  int pri[168];    // a pri threshhold for each hour of the week
  int currentpri;
  int currentcpus;
  pid_t updatepid;      // process responsible for updating
  time_t lastresponse;  // timestamp of last network response
  string hostname, nickname;
  struct sockaddr_in addr;
  int speed;    // FIXME not used?
  int rank;     // lower rank means use me first
  uint32 rand;  // random number for mixing up host order
  int total_cpus, taken_cpus, avail_cpus;
  int socket;
  string status;  // up, dead, down, or unknown
  int valid;
  vector<string> checkdirs;

  VBHost();
  VBHost(const string nn, const string hn, uint16 serverport);
  int ReadFile(const string &fname);
  void init();
  void setnames(const string nn, const string hn = "");
  void initaddress(uint16 serverport);
  void print();
  void Update();
  void CheckSchedule();
  void Ping(map<jobid, VBJobSpec> &runningmap);
  int SendMsg(string msg);
  void DeadOrAlive();
  float LoadAverage() const;
  void SetLoadAverage(float load);
  short Priority() const;
  string tobuffer(map<jobid, VBJobSpec> &runningmap);
  int frombuffer(string buf);
  void GetHost();
  void updateresources();
};

class VBenchmark {
 public:
  string name;
  time_t interval;   // interval in s, e.g., 1800 (30mins)
  time_t scheduled;  // scheduled time to run it
  string cmd;        // command to run and time
};

class VBPrefs {
 public:
  VBPrefs();
  // config directories, etc.
  string rootdir;                     // full path to top level voxbo dir
  string homedir;                     // user's home directory
  string userdir;                     // user's voxbo prefs/config directory
  string queuedir;                    // THE voxbo queue
  string email;                       // user's email address
  string username;                    // user's voxbo name (also unix name)
  VBHost thishost;                    // server data for this host if available
  int32 cores;                        // number of cores to use on THIS
                                      // MACHINE (should be set to 0 if
                                      // we're queueing)
  int su;                             // is user privileged?
  uint16 serverport;                  // port to which the server listens
  map<string, VBJobType> jobtypemap;  // all our jobtypes, mapped by name

  // stuff that's only needed by the scheduler/servers
  string sysadmin;              // who to email in case of disaster
  set<string> superusers;       // users with special powers
  list<VBenchmark> benchmarks;  // specs for occasional benchmark to run
  int queuedelay;               // how long between checking queue in seconds?
  uid_t voxbouid;               // uid of the voxbo user
  gid_t voxbogid;               // gid of the voxbo user
  string sendmail;              // full path to sendmail program
  map<string, string> servers;  // initial list of servers to contact

  void init();
  void read_prefs(FILE *fp, int main = 0);
  void read_jobtypes();
  int read_serverfile();
  void set_queue(string name, string dir, ino_t inode);
};

class VBResource {
 private:
 public:
  string name;     // the arbitrary name for this resource
  string command;  // command that will return integer reflecting availability
  string host;
  int f_global;  // is this resource cluster-global or machine-local
  int cnt;
  int current;
  VBResource();
};

class VBReservation {
 private:
 public:
  string owner;
  time_t start;
  time_t end;
  string reason;
};

// a VBAction encodes things to be done after a command runs, based on
// return codes or text output.  a command may have more than one
// action. as soon as the job's disposition is determined, it's done.
// any command that doesn't otherwise have an action gets the default,
// which is "?notreturn 0 fail"

// SYNTAX:
// ?match "pat" fail
// ?return 57 redo <n>
// ?notreturn 0 fail
// ?match "Percent done:" tagjob pct $3

bool cmp_host_pri_taken(const VBHost &h1, const VBHost &h2);
bool cmp_host_name(const VBHost &h1, const VBHost &h2);
bool cmp_host_random(const VBHost &, const VBHost &);

// communicate with scheduler, these functions are in vbhost.cpp

// int tell_scheduler(const VBPrefs &vbp,string msg,string &returnmsg);

// some iterators

typedef map<string, VBJobType>::iterator TI;
typedef vector<VBTrigger>::iterator TGI;
typedef list<VBHost>::iterator HI;
// extern VBPrefs vbp;

#endif  // VBPREFS_H
