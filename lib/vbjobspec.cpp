
// vbjobspec.cpp
// VoxBo job spec structures
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

#include "vbjobspec.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "vbprefs.h"
#include "vbutil.h"

// JobSpecSorter() is the comparison function used for sorting jobs
// into the order in which they should be tried.  1 means j1 goes
// first, 0 means j2.  note that operator< is now used, because it's
// used in lists, and not vectors.

bool JobSpecSorter(const VBJobSpec &j1, const VBJobSpec &j2) {
  if (j1.priority > j2.priority) return 1;
  if (j1.priority < j2.priority) return 0;
  if (j1.snum < j2.snum) return 1;
  if (j1.snum > j2.snum) return 0;
  if (j1.jnum < j2.jnum) return 1;
  if (j1.jnum > j2.jnum) return 0;
  return 0;
}

bool operator<(const VBJobSpec &j1, const VBJobSpec &j2) {
  if (j1.priority > j2.priority) return 1;
  if (j1.priority < j2.priority) return 0;
  if (j1.snum < j2.snum) return 1;
  if (j1.snum > j2.snum) return 0;
  if (j1.jnum < j2.jnum) return 1;
  if (j1.jnum > j2.jnum) return 0;
  return 0;
}

bool SeqSorter(const VBSequence &s1, const VBSequence &s2) {
  if (s1.seqnum < s2.seqnum)
    return 1;
  else
    return 0;
  return 0;
}

bool operator<(const VBSequence &s1, const VBSequence &s2) {
  if (s1.seqnum < s2.seqnum)
    return 1;
  else
    return 0;
  return 0;
}

bool SeqAllDone(const VBSequence &s) {
  if (s.status == 'X')
    return 1;
  else
    return 0;
}

//////////////////////
// VBSequence stuff
//////////////////////

VBSequence::VBSequence() { init(); }

void VBSequence::init() {
  specmap.clear();
  name = "";
  owner = "";
  uid = getuid();
  email = "";
  waitfor.clear();
  forcedhosts.clear();
  valid = 0;
  seqnum = 0;
  jobcnt = badcnt = donecnt = waitcnt = runcnt = 0;
  queuedtime = 0;
  status = 'R';
  seqdir = "";
  modtime = 0;

  priority.init();
}

VBSequence::VBSequence(string seqname, int jobnum) {
  init();
  LoadSequence(seqname, jobnum);
}

VBSequence::VBSequence(const VBPrefs &vbp, int seqnum, int jobnum) {
  init();
  string sp = findseqpath(vbp.queuedir, seqnum);
  if (sp.size()) LoadSequence(sp, jobnum);
}

int VBSequence::LoadSequence(string sdir, int jobnum) {
  FILE *sfp;
  char line[STRINGLEN], tmp[STRINGLEN];
  tokenlist args;
  struct stat st;

  init();
  seqdir = sdir;
  if (stat((seqdir + "/info.seq").c_str(), &st)) return 99;
  modtime = st.st_mtime;
  if ((sfp = fopen((seqdir + "/info.seq").c_str(), "r")) ==
      NULL)  // open the seq file
    return 111;
  seqnum = 0;

  while (fgets(line, STRINGLEN, sfp) != NULL) {
    ParseSeqLine(line);
    continue;
  }
  fclose(sfp);

  // assume we're okay
  valid = 1;

  // check for sequence waitfor files
  vglob vg(seqdir + "/*.wait");
  for (size_t i = 0; i < vg.size(); i++) {
    string str = xfilename(vg[i]);
    int wf = strtol(str);
    if (wf > 0) waitfor.insert(wf);
  }

  if (jobnum != -2) {
    sprintf(tmp, "%s/*.job", seqdir.c_str());
    // if only one requested, use that pattern
    if (jobnum > -1) sprintf(tmp, "%s/%05d.job", seqdir.c_str(), jobnum);
    vg.load(tmp);
    int start = 0;
    if (jobnum == -3) start = vg.size() - 1;
    for (size_t i = start; i < vg.size(); i++) {
      VBJobSpec js;
      if (stat(vg[i].c_str(), &st)) continue;
      if (st.st_mtime > modtime) modtime = st.st_mtime;
      if (js.ReadFile(vg[i])) continue;
      // copy some globals from the sequence info to the job info
      js.email = email;
      js.seqname = name;
      js.uid = uid;
      js.snum = seqnum;
      js.owner = owner;
      js.priority = priority.priority;
      js.forcedhosts = forcedhosts;
      // check if job nums match their order
      if (jobnum == -1 && js.jnum != (int)specmap.size()) return 191;
      // now add it to the list if appropriate
      specmap[js.jnum] = js;
    }
    updatecounts();
  }
  return (0);  // no error!
}

int VBSequence::ParseSeqLine(string line) {
  // FIXME for some of these fields, in principle we would need to
  // update the jobs as well.  for now, not a problem since this is
  // only called either when loading a whole sequences (jobs loaded
  // later) or in the server (only non-sensitive fields affected)
  tokenlist args;
  args.ParseLine(line);
  if (args.size() < 2)  // everything requires an argument
    return 1;
  else if (args[0] == "name")
    name = args.Tail();
  else if (args[0] == "source")
    source = args.Tail();
  else if (args[0] == "email")
    email = args[1];
  else if (args[0] == "seqnum")
    seqnum = strtol(args[1]);
  else if (args[0] == "uid")
    uid = strtol(args[1]);
  // FIXME make sure the following new items work!
  else if (args[0] == "require") {
    if (args.size() > 2)
      requires[args[1]] = strtol(args[2]);
    else
      requires[args[1]] = 0;
  } else if (args[0] == "priority")
    priority.priority = strtol(args[1]);
  else if (args[0] == "maxjobs")
    priority.maxjobs = strtol(args[1]);
  else if (args[0] == "maxperhost")
    priority.maxperhost = strtol(args[1]);
  else if (args[0] == "priority2")
    priority.priority2 = strtol(args[1]);
  else if (args[0] == "maxjobs2")
    priority.maxjobs2 = strtol(args[1]);
  else if (args[0] == "forcedhost")
    forcedhosts.insert(args[1]);
  else if (args[0] == "owner") {
    owner = args[1];
    if (!email[0]) email = owner;
  } else if (args[0] == "queuedtime")
    queuedtime = strtol(args[1]);
  else if (args[0] == "status")
    status = args[1][0];
  return 0;
}

void VBSequence::updatecounts() {
  runcnt = waitcnt = badcnt = donecnt = jobcnt = 0;
  for (SMI i = specmap.begin(); i != specmap.end(); i++) {
    if (i->second.status == 'W')
      waitcnt++;
    else if (i->second.status == 'S')
      runcnt++;
    else if (i->second.status == 'R')
      runcnt++;
    else if (i->second.status == 'B')
      badcnt++;
    else if (i->second.status == 'D')
      donecnt++;
    jobcnt++;
  }
}

// writes a sequence file from data, tacks on .tmp to the requested
// filename and then rename()s, to avoid race conditions

int VBSequence::Write(string dirname) {
  FILE *fp;
  char tmpname[STRINGLEN];

  // create the directory
  if (mkdir(dirname.c_str(), 0777)) return (101);
  string seqname1 = dirname + "/info.tmpseq";
  string seqname2 = dirname + "/info.seq";

  // create the sequence file
  fp = fopen(seqname1.c_str(), "w");
  if (!fp) return (102);

  fprintf(fp, "status %c\n", status);
  fprintf(fp, "name %s\n", name.c_str());
  fprintf(fp, "source %s\n", source.c_str());
  fprintf(fp, "owner %s\n", owner.c_str());
  fprintf(fp, "uid %d\n", (int)uid);
  fprintf(fp, "seqnum %d\n", seqnum);
  if (queuedtime) fprintf(fp, "queuedtime %ld\n", queuedtime);
  vbforeach(string fh, forcedhosts) fprintf(fp, "forcedhost %s\n", fh.c_str());
  fprintf(fp, "email %s\n", email.c_str());
  fprintf(fp, "maxjobs %d\n", priority.maxjobs);
  fprintf(fp, "priority %d\n", priority.priority);
  fprintf(fp, "maxjobs2 %d\n", priority.maxjobs2);
  fprintf(fp, "priority2 %d\n", priority.priority2);
  fprintf(fp, "maxperhost %d\n", priority.maxperhost);
  for (map<string, int>::iterator rr = requires.begin(); rr != requires.end();
       rr++)
    fprintf(fp, "require %s %d\n", rr->first.c_str(), rr->second);
  fclose(fp);

  // now renumber all the jobs and create each job file

  int errs = 0;
  renumber(0);
  for (SMI js = specmap.begin(); js != specmap.end(); js++) {
    sprintf(tmpname, "%s/%05d.job", dirname.c_str(), js->first);
    if (js->second.Write(tmpname)) errs++;
  }
  if (errs) {
    rmdir_force(dirname);
    return (120);
  }
  rename(seqname1.c_str(), seqname2.c_str());
  return 0;
}

int VBJobSpec::Write(string fname) {
  FILE *fp;
  fp = fopen(fname.c_str(), "w");
  if (!fp) return 101;
  fprintf(fp, "status %c\n", status);
  fprintf(fp, "name %s\n", name.c_str());
  fprintf(fp, "jnum %d\n", jnum);
  fprintf(fp, "dirname %s\n", dirname.c_str());
  if (logdir.size()) fprintf(fp, "logdir %s\n", logdir.c_str());
  fprintf(fp, "jobtype %s\n", jobtype.c_str());

  if (waitfor.size())
    fprintf(fp, "waitfor %s\n", textnumberset(waitfor).c_str());

  if (finishedtime) fprintf(fp, "finishedtime %ld\n", finishedtime);
  if (startedtime) fprintf(fp, "startedtime %ld\n", startedtime);
  if (serverfinishedtime)
    fprintf(fp, "serverfinishedtime %ld\n", serverfinishedtime);
  if (serverstartedtime)
    fprintf(fp, "serverstartedtime %ld\n", serverstartedtime);
  if (percentdone > -1) fprintf(fp, "percentdone %d\n", percentdone);
  if (magnitude != 0) fprintf(fp, "magnitude %ld\n", magnitude);
  if (hostname.size()) fprintf(fp, "host %s\n", hostname.c_str());
  pair<string, string> pp;
  vbforeach(pp, arguments)
      fprintf(fp, "argument %s %s\n", pp.first.c_str(), pp.second.c_str());

  fprintf(fp, "\n# end of job definition\n\n");

  fclose(fp);
  return 0;  // no error!
}

// FIXME -- submit really needs to just send the whole sequence over
// the socket

vbreturn VBSequence::Submit(const VBPrefs &vbp) {
  int err;
  string sname = vbp.rootdir + "/drop/tmp_" + vbp.username + "_" +
                 uniquename(vbp.thishost.nickname);
  string sname2 = vbp.rootdir + "/drop/submit_" + vbp.username + "_" +
                  uniquename(vbp.thishost.nickname);
  mode_t oldumask = umask(0);
  owner = vbp.username;
  err = Write(sname);
  if (err) {
    umask(oldumask);
    return vbreturn(101, "error writing temporary sequence file");
  }
  rename(sname.c_str(), sname2.c_str());
  umask(oldumask);
  string ret;
  // FIXME the following removed, we no longer notify scheduler on drop
  // err=tell_scheduler((string)"SUBMIT "+sname,ret);
  // if (err)
  //   return vbreturn(err,"error telling scheduler about the new sequence");
  // if (ret[1]=='E')
  //   return vbreturn(102,ret.substr(4));
  // rmdir_force(sname);
  return 0;
}

void VBSequence::addJob(VBJobSpec &job) { specmap[job.jnum] = job; }

int VBSequence::renumber(int firstjnum) {
  // first pass, create the map of old job number to new job number, and update
  // jnums
  map<int, int> jmap;
  int renumberneeded = 0;
  int jnum;
  int index = 0;
  for (SMI i = specmap.begin(); i != specmap.end(); i++) {
    jnum = i->first;
    jmap[jnum] = index + firstjnum;
    if (jnum != index + firstjnum) renumberneeded = 1;
    i->second.jnum = index + firstjnum;
    index++;
  }
  if (!renumberneeded) return specmap.size();
  // second pass, redo the map
  map<int, VBJobSpec> tmp;
  for (SMI i = specmap.begin(); i != specmap.end(); i++)
    tmp[i->second.jnum] = i->second;
  specmap.swap(tmp);
  // final pass, renumber the waitfors
  vector<int>::iterator ii;
  for (SMI i = specmap.begin(); i != specmap.end(); i++) {
    set<int32> newwaitfors;
    vbforeach(int32 ww, i->second.waitfor) newwaitfors.insert(jmap[ww]);
    i->second.waitfor = newwaitfors;
  }
  return specmap.size();
}

// FIXME NOT USED AT ALL!  ParseSummary() and GetSummary() are used to
// pass around summary information about a sequence -- usually the
// sequence instance used is a shell with no other content

int VBSequence::ParseSummary(string str) {
  tokenlist args, line;
  string val;
  args.SetSeparator("\n");
  args.ParseLine(str);
  for (size_t i = 0; i < args.size(); i++) {
    line.ParseLine(args[i]);
    val = line.Tail();
    if (line.size() < 1) continue;
    if (line[0] == "name")
      name = val;
    else if (line[0] == "owner")
      owner = val;
    else if (line[0] == "uid")
      uid = strtol(val);
    else if (line[0] == "email")
      email = val;
    else if (line[0] == "waitfor")
      waitfor = numberset(val);
    else if (line[0] == "forcedhost")
      forcedhosts.insert(val);
    else if (line[0] == "valid")
      valid = strtol(val);
    else if (line[0] == "seqnum")
      seqnum = strtol(val);
    else if (line[0] == "jobcnt")
      jobcnt = strtol(val);
    else if (line[0] == "badcnt")
      badcnt = strtol(val);
    else if (line[0] == "donecnt")
      donecnt = strtol(val);
    else if (line[0] == "waitcnt")
      waitcnt = strtol(val);
    else if (line[0] == "runcnt")
      runcnt = strtol(val);
    else if (line[0] == "queuedtime")
      queuedtime = strtol(val);
    else if (line[0] == "status")
      status = val[0];
    else if (line[0] == "seqdir")
      seqdir = val;
    else if (line[0] == "source")
      source = val;
    else if (line[0] == "requires")
      requires[line[1]] = strtol(line[2]);
    else if (line[0] == "priority")
      priority.init(val);
    else if (line[0] == "effectivepriority")
      effectivepriority = strtol(val);
  }
  return 0;
}

string VBSequence::GetSummary() {
  string ret;
  ret += "name " + name + "\n";
  ret += "owner " + owner + "\n";
  ret += "uid " + strnum(uid) + "\n";
  ret += "email " + email + "\n";
  ret += "waitfor " + textnumberset(waitfor) + "\n";
  vbforeach(string h, forcedhosts) ret += "forcedhost " + h + "\n";
  ret += "valid " + strnum(valid) + "\n";
  ret += "seqnum " + strnum(seqnum) + "\n";
  ret += "jobcnt " + strnum(jobcnt) + "\n";
  ret += "badcnt " + strnum(badcnt) + "\n";
  ret += "donecnt " + strnum(donecnt) + "\n";
  ret += "waitcnt " + strnum(waitcnt) + "\n";
  ret += "runcnt " + strnum(runcnt) + "\n";
  ret += "queuedtime " + strnum(queuedtime) + "\n";
  ret += (format("status %c\n") % status).str();
  ret += "seqdir " + seqdir + "\n";
  ret += "source " + source + "\n";
  pair<string, int> rr;
  vbforeach(rr, requires) ret +=
      "requires " + rr.first + " " + strnum(rr.second) + "\n";
  ret += "priority " + (string)priority + "\n";
  ret += "effectivepriority " + strnum(effectivepriority) + "\n";
  return ret;
}

string findseqpath(string queuedir, int seqnum) {
  string tmp = (format("%s/%08d") % queuedir % seqnum).str();
  if (vb_direxists(tmp))
    return tmp;
  else
    return "";
}

VBJobSpec::VBJobSpec() { init(); }

void VBJobSpec::init() {
  name = hostname = "";
  seqname = "anon vb seq";
  logdir = "";
  dirname = "/tmp";
  email = errorstring = jobtype = "";
  owner = "";
  magnitude = error = 0;
  startedtime = 0;
  finishedtime = 0;
  serverstartedtime = 0;
  serverfinishedtime = 0;
  status = 'W';
  uid = 0;
  pid = childpid = 0;
  maxcpus = 1;
  actualcpus = 1;
  percentdone = -1;
  jt.init();
  jnum = -1;
  snum = -1;
  priority = 0;
  arguments.clear();
  waitfor.clear();
  f_cluster = 1;
}

string VBJobSpec::basename() {
  return (format("%08d-%08d") % snum % jnum).str();
}

string VBJobSpec::logfilename() {
  return (format("%s/%s.log") % logdir % basename()).str();
}

string VBJobSpec::seqdirname() { return (format("%08d") % snum).str(); }

int VBJobSpec::ReadFile(string fname) {
  init();
  FILE *fp = fopen(fname.c_str(), "r");
  if (!fp) return 101;
  jnum = strtol(xfilename(fname));

  char line[STRINGLEN];
  while (fgets(line, STRINGLEN, fp) != NULL) ParseJSLine((string)line);

  fclose(fp);
  return 0;
}

void VBJobSpec::ParseJSLine(string str) {
  str = xstripwhitespace(str);
  if (str[0] == '#' || str[0] == '%' || str[0] == ';') return;
  tokenlist args;
  args.SetQuoteChars("");
  args.ParseLine(str);
  if (args.size() == 0)  // skip blank lines
    return;
  // the following lets us ignore empty arguments
  if (args.size() < 2 && args[0] != "argument") return;
  if (args[0] == "name")
    name = args.Tail();
  else if (args[0] == "jnum")
    jnum = strtol(args[1]);
  else if (args[0] == "argument") {
    tokenlist aa;
    // aa.SetSeparator("=");
    aa.ParseLine(args.Tail());
    arguments[aa[0]] = aa.Tail();
  } else if (args[0] == "dirname")
    dirname = args[1];
  else if (args[0] == "jobtype")
    jobtype = args[1];
  else if (args[0] == "status") {
    status = args[1][0];
  } else if (args[0] == "waitfor") {
    for (size_t k = 1; k < args.size(); k++) {
      vector<int> tmpl = numberlist(args[k]);
      for (int l = 0; l < (int)tmpl.size(); l++) waitfor.insert(tmpl[l]);
    }
  } else if (args[0] == "startedtime")
    startedtime = strtol(args[1]);
  else if (args[0] == "finishedtime")
    finishedtime = strtol(args[1]);
  else if (args[0] == "serverstartedtime")
    serverstartedtime = strtol(args[1]);
  else if (args[0] == "serverfinishedtime")
    serverfinishedtime = strtol(args[1]);
  else if (args[0] == "pid")
    pid = strtol(args[1]);
  else if (args[0] == "childpid")
    childpid = strtol(args[1]);
  else if (args[0] == "percentdone")
    percentdone = strtol(args[1]);
  else if (args[0] == "host")
    hostname = args[1];
  else if (args[0] == "magnitude")
    magnitude = strtol(args[1]);
  else if (args[0] == "logdir")
    logdir = args[1];
}

void VBJobSpec::SetState(JobState s) {
  state = s;
  char c = ' ';
  if (state == XBad) c = 'B';
  if (state == XGood) c = 'G';
  if (state == XSignal) c = 'S';
  if (state == XWarn) c = 'W';
  if (state == XRetry) c = 'R';
  if (state == XNone) c = 'N';
}

JobState VBJobSpec::GetState() { return state; }

void VBJobSpec::print() {
  printf("JOBSPEC %s (%s)\n", name.c_str(), basename().c_str());
  printf("    jobtype: %s\n", jobtype.c_str());
  printf("working dir: %s\n", dirname.c_str());
  printf("      owner: %s (uid %d, email %s)\n", owner.c_str(), (int)uid,
         email.c_str());
  printf("   priority: %d\n", priority);
  printf("   sequence: %s\n", seqname.c_str());
  printf("     status: %c\n", status);
  pair<string, string> pp;
  vbforeach(pp, arguments)
      printf(" argument: %s=%s\n", pp.first.c_str(), pp.second.c_str());
}

void VBSequence::print() {
  printf("SEQUENCE %s (%d)\n", name.c_str(), seqnum);
  printf("owner: %s\n", owner.c_str());
  printf("  dir: %s\n", seqdir.c_str());
  printf(" jobs: %d\n", jobcnt);
  printf("  run: %d\n", runcnt);
  printf(" wait: %d\n", waitcnt);
  printf(" done: %d\n", donecnt);
  printf("  bad: %d\n", badcnt);
  printf("  pri: %d\n", priority.priority);
  printf("  max: %d\n", priority.maxjobs);
  printf(" pri2: %d\n", priority.priority2);
  printf(" max2: %d\n", priority.maxjobs2);
  printf(" maxperhost: %d\n", priority.maxperhost);
  printf(" modtime: %d\n", (int)modtime);
}

void VBpri::init(string str) {
  if (str.size() != 10) str = "0003000000";
  maxjobs = strtol(str.substr(0, 2));
  priority = strtol(str.substr(2, 2));
  maxjobs2 = strtol(str.substr(4, 2));
  priority2 = strtol(str.substr(6, 2));
  maxperhost = strtol(str.substr(8, 2));
}

VBpri::operator const string() {
  string str;
  str += (boost::format("%s jobs at pri %d") %
          (maxjobs ? strnum(maxjobs) : "unlimited") % priority)
             .str();
  if (maxjobs > 0 && priority2 > 0)
    str += (boost::format(", %s jobs at pri %d") %
            (maxjobs2 ? strnum(maxjobs2) : "unlimited") % priority2)
               .str();
  if (maxperhost > 0)
    str +=
        (boost::format(", no more than %d jobs per server") % maxperhost).str();
  return str;
}

void VBpri::operator=(const uint16 pri) {
  init();
  priority = pri;
  if (priority > 5) priority = 5;
}

int VBpri::set(const string pri) {
  init();
  tokenlist tmp;
  tmp.ParseLine(pri);
  return set(tmp);
}

// currently understands single pri, single preset, or all five params

int VBpri::set(tokenlist &args) {
  init();
  if (args.size() == 1) {
    string str = vb_tolower(args[0]);
    if (str == "default")
      init("0003000000");
    else if (str == "offhours")
      init("0001000000");
    else if (str == "nice")
      init("0403000200");
    else if (str == "xnice")
      init("0202000100");
    else if (str == "hold")
      init("0000000000");
    else if (str == "0" || str == "1" || str == "2" || str == "3" ||
             str == "4" || str == "5")
      priority = strtol(str);
    else {
      return 1;
    }
  } else if (args.size() == 2) {
    maxjobs = strtol(args[0]);
    priority = strtol(args[1]);
  } else if (args.size() == 4) {
    maxjobs = strtol(args[0]);
    priority = strtol(args[1]);
    maxjobs2 = strtol(args[2]);
    priority2 = strtol(args[3]);
  } else if (args.size() == 5) {
    maxjobs = strtol(args[0]);
    priority = strtol(args[1]);
    maxjobs2 = strtol(args[2]);
    priority2 = strtol(args[3]);
    maxperhost = strtol(args[4]);
  } else
    return 1;
  return 0;  // no error!
}

// string const
// VBpri::tobuf()
// {
//   return (boost::format("%d %d %d %d
//   %d")%maxjobs%priority%maxjobs2%priority2%maxperhost).str();
// }

// int
// VBpri::frombuf(const string &str)
// {
//   init();
//   tokenlist toks;
//   toks.ParseLine(str);
//   if (toks.size()!=5) return 111;
//   maxjobs=strtol(toks[0]);
//   priority=strtol(toks[1]);
//   maxjobs2=strtol(toks[2]);
//   priority2=strtol(toks[3]);
//   maxperhost=strtol(toks[4]);
//   if (priority>5) priority=5;
//   if (priority2>5) priority2=5;
//   return 0;
// }
