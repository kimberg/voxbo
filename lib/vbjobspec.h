
// vbjobspec.h
// class def, prototypes, etc. for vbjobspec
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

#ifndef VBJOBSPEC_H
#define VBJOBSPEC_H

#include <vector>
#include <list>
#include "tokenlist.h"
#include "vbutil.h"
//#include "vbprefs.h"

#define MAXFILENAME 128
#define SECONDSINADAY 86400           // number of seconds in a day
#define SECONDSINAWEEK 604800         // number of second in a week
#define RESEND_WAIT 120               // seconds before a job is resent

class VBPrefs;
class VBSequence;
class VBJobType;
class VBJobSpec;
typedef vector<VBJobSpec>::iterator JI;
typedef list<VBJobSpec>::iterator JLI;
typedef map<int,VBSequence>::iterator SI;
typedef map<int,VBJobSpec>::iterator SMI;
enum JobState {XGood,XBad,XWarn,XRetry,XSignal,XNone};

class VBpri {
public:
  VBpri() {init();}
  void init(string str="");
  operator const string();
  void operator=(const uint16 pri);
  int set(tokenlist &args);
  int set(const string pri);
  uint16 priority;
  uint16 maxjobs;
  uint16 maxperhost;
  uint16 priority2;
  uint16 maxjobs2;
};

class VBArgument {
 public:
  string name;
  string type;
  string description;
  string defaultval;
  string low;
  string high;
  string role;
};

class VBTrigger {
public:
  string cond;           // match, return, or notreturn
  string condvalue;      // pattern or return code
  string action;         // fail, warn, redo, redo n, 
  string actionvalue;
};

struct jobdata {
  string key;
  vector<string> datalist;
};

class VBJobType {
 public:
  string shortname;            // "realign" no spaces!
  string description;          // "Realign 3- or 4-dimensional data to a template"
  string invocation;           // how it's invoked (builtin, script, commandline)
  vector<VBArgument> arguments;
  vector<jobdata> jobdatalist;  // FIXME use multimap instead?
  tokenlist getData(string key);
  // bool usertype;
  class VBcmd {
  public:
    string command;              // command to execute
    vector<string> script;       // lines of the script
  };
  vector<VBcmd> commandlist;     // vector of commands
  vector<VBTrigger> triggerlist; // actions to take when job is complete
  vector<string> setenvlist;     // environment variables set for this job

  // the four types of lines we need to detect
  string err_tag;           // string at the beginning of error lines
  string warn_tag;          // " warning lines
  string msg_tag;           // " informative message lines
  string retry_tag;         // " lines that mean we should try again later

  // requirements
  map<string,int> requires;
  vector<string> nomail;   // lines not to mail to the user
  
  VBJobType();

  void init();
  void print();
  int ReadJOB1(const string &fname);
};



class VBJobSpec {
 private:
  JobState state;
 public:
  string name;               // a text string, the name of the job
  string dirname;            // working directory
  map<string,string> arguments; // arguments used in invoking job
  string jobtype;            // just the first token of the commandline
  set<int32> waitfor;        // list of job nums to wait for
  string logdir;             // full path to log file
  string seqname;            // the inherited sequence name
  string email;              // address of owner
  string owner;
  string errorstring;        // returned error message
  string hostname;           // host running the job
  set<string> forcedhosts;
  // FIXME the following three fields used for transport
  uid_t voxbouid;
  gid_t voxbogid;
  string queuedir;

  bool f_cluster;             // in cluster mode we send msgs to scheduler

  VBJobType jt;                       // the actual jobtype, not just its name
  int32 snum,jnum;                    // sequence and job numbers
  int error,priority;
  time_t startedtime;                 // scheduler time job was sent to server
  time_t finishedtime;                // scheduler time job finished
  time_t serverstartedtime;           // server time job started
  time_t serverfinishedtime;          // server time job finished
  // time_t laststatusupdate;            // last status update
  time_t lastreport;                  // last time server reported it as running
  long magnitude;                     // magnitude of the job, in any units
  int retrycount;                     // number of jobs to redo based on recent VBTrigger
  int maxcpus,actualcpus;             // max and actual number of cpus
  pid_t pid,childpid;                 // pid of running vbsrvd and its child
  uid_t uid;
  char status;
  int percentdone;                    // percent done, if available

  VBJobSpec();
  void init();
  int Write(string fname);
  int ReadFile(string fname);
  void ParseJSLine(string str);
  void SetState(JobState s);
  JobState GetState();
  void print();
  string basename();           // something like 000000_000000
  string seqdirname();         // something like 000000
  string logfilename();        // logdir/basename().log
};

bool operator<(const VBJobSpec &j1,const VBJobSpec &j2);
bool operator<(const VBSequence &s1,const VBSequence &s2);

class VBSequence {
 public:
  // when something is added in here, we need to update init(),
  // LoadSequence(), ParseSeqLine(), Get/ParseSummary(), Write() ...
  map<int,VBJobSpec> specmap;
  string name;               // sequence name
  string owner;              // name of owner (userid)
  uid_t uid;                 // uid of owner
  string email;              // user email address
  set<int32> waitfor;        // list of sequences to wait for
  set<string> forcedhosts;   // only run on these
  int valid;                 // read successful
  int seqnum;                // unique sequence number
  int jobcnt,badcnt,donecnt,waitcnt,runcnt;
  long queuedtime;
  time_t modtime;
  char status;        // R=ready P=postponed K=killed X=private M=moved  FIXME enum
  string seqdir;      // location of sequence files
  string source;      // name/path of script file, vbbatch command line, etc.
  map<string,int> requires;

  VBpri priority;
  int effectivepriority;

  // schedule stuff

  VBSequence();
  VBSequence(string seqname,int jobnum=-1);
  VBSequence(const VBPrefs &vbp,int seqnum,int jobnum=-1);
  void addJob(VBJobSpec &job);
  void addJobs(vector<VBJobSpec> &jobs);
  int LoadSequence(string seqdir,int jobnum=-1);
  int Write(string dirname);
  vbreturn Submit(const VBPrefs &vbp);
  int renumber(int firstjnum);
  void init();
  int ParseSummary(string str);  // FIXME unused???
  int ParseSeqLine(string line);
  string GetSummary();           // FIXME unused???
  void updatecounts();
  void print();
};

string findseqpath(string queuedir,int seqnum);

#endif // VBJOBSPEC_H

