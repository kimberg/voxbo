
// voxq.cpp
// VoxBo queue utility
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

#include <netdb.h>
#include <sys/un.h>
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbutil.h"
#include "voxq.hlp.h"

using namespace std;
using boost::format;

// GLOBALS
VBPrefs vbp;
list<VBHost> hostlist;
list<VBSequence> seqlist;

int voxq_sequences(tokenlist &args, set<string> flags);
void voxq_sequencedetail(tokenlist &args);
void voxq_debug(tokenlist &args);
void voxq_dumpconfig(tokenlist &args);
void voxq_dumpjobtypes(tokenlist &args);
void voxq_marksequence(tokenlist &args, string status);
void voxq_killsequence(tokenlist &args, string newstatus);
void voxq_changejobs(tokenlist &args, char from, char to);
void voxq_help();
void voxq_gimme(tokenlist &args, int gim = TRUE);
void voxq_cpus(string modename);
void voxq_resources();
void voxq_die();
void voxq_reset();
void voxq_server(tokenlist &args);
void voxq_queue(tokenlist &args);
void voxq_version();
void voxq_whynot(tokenlist &args);
void voxq_sched(tokenlist &args);
void quit_handler(int);
void server2host(string servername, char *hostname);
int voxq_send_file(tokenlist &args);
int get_scheduler_info(string msg, string &info);
int get_hostlist();
int get_sequencelist();
void add_numbers(set<int32> &nums, string str);
void print_queue(int argc, char **argv);
int voxq_tell_scheduler(string msg, string &returnmsg);

int get_addr_unix(struct sockaddr_un &addr);
int get_addr_inet(struct sockaddr_in &addr);

// uid_t euid;

int main(int argc, char *argv[]) {
  umask(022);
  vbp.init();
  vbp.read_serverfile();

  tokenlist args, emptylist;
  args.Transfer(argc - 1, argv + 1);
  string modename = vb_tolower(args[0]);
  if (modename.substr(0, 2) == "--")
    modename = modename.substr(2, string::npos);
  else if (modename[0] == '-')
    modename = modename.substr(1, string::npos);
  args[0] = modename;
  set<string> flags;
  // flags.insert("all");

  int ret = 0;
  if (args.size() < 1 || modename == "h" || modename == "help")
    voxq_help();
  else if (modename == "all" || modename == "a") {
    voxq_cpus(modename);
    voxq_sequences(emptylist, flags);
  } else if (modename == "version" || modename == "v")
    voxq_version();
  else if (modename == "seqs" || modename == "s")
    ret = voxq_sequences(args, flags);
  else if (modename == "ss") {
    flags.insert("all");
    ret = voxq_sequences(args, flags);
  } else if (modename == "resources" || modename == "l")
    voxq_resources();
  else if (modename == "x")
    voxq_sequencedetail(args);
  else if (modename == "xx")
    voxq_sequencedetail(args);
  else if (modename == "cpus" || modename == "c")
    voxq_cpus(modename);
  else if (modename == "gimme" || modename == "g")
    voxq_gimme(args, TRUE);
  else if (modename == "giveback" || modename == "b")
    voxq_gimme(args, FALSE);
  else if (modename == "kill" || modename == "k")
    voxq_killsequence(args, "K");  // kill jobs, mark to be removed
  else if (modename == "postpone" || modename == "p")
    voxq_marksequence(args, "P");  // postpone jobs
  else if (modename == "resume" || modename == "r")
    voxq_marksequence(args, "R");  // mask as ready
  else if (modename == "retry" || modename == "t")
    voxq_changejobs(args, 'B', 'W');
  else if (modename == "pushthrough" || modename == "u")
    voxq_changejobs(args, 'B', 'D');
  else if (modename == "whynot" || modename == "j")
    voxq_whynot(args);
  else if (modename == "halt" || modename == "y")
    voxq_killsequence(args, "P");  // kill jobs, mark as P
  else if (modename == "w")
    voxq_changejobs(args, '*', 'W');
  else if (modename == "d")
    voxq_debug(args);
  // pri/max stuff
  else if (modename == "sched")
    voxq_sched(args);
  else if (modename == "max" || modename == "m")
    voxq_sched(args);
  else if (modename.size() == 1 && strchr("012345", modename[0]))
    voxq_sched(args);

  // SYSADMIN ONLY
  else if (modename == "die")
    voxq_die();
  else if (modename == "reset")
    voxq_reset();
  else if (modename == "server")
    voxq_server(args);
  else if (modename == "queue")
    voxq_queue(args);
  else if (modename == "dump")
    voxq_dumpconfig(args);
  else if (modename == "jobtypes")
    voxq_dumpjobtypes(args);
  else {
    printf("voxq: unrecognized options, try voxq -help for help\n");
    exit(5);
  }

  exit(ret);
}

enum {
  vb_notfound = 0,
  vb_running = 1,
  vb_postponed = 2,
  vb_killed = 3,
  vb_bad = 4,
  vb_waiting = 5,
  vb_nodata = 6
};

int voxq_sequences(tokenlist &args, set<string> flags) {
  if (args.size() > 2) {
    voxq_help();
    exit(100);
  }
  int myseq = 0, ret = vb_notfound;
  if (args.size() == 2) myseq = strtol(args[1]);
  if (seqlist.size() == 0) {
    if (get_sequencelist()) {
      printf("[E] voxq: error retrieving queue data\n");
      return vb_nodata;
    }
  }
  if (seqlist.size() < 1) {
    printf("The VoxBo queue is empty.\n");
    return vb_notfound;
  }
  seqlist.sort();

  int diecnt = 0, postcnt = 0;
  int i = 0, skippedcnt = 0;
  int runcnt = 0, waitcnt = 0, badcnt = 0, donecnt = 0, jobcnt = 0, seqcnt = 0;
  for (list<VBSequence>::iterator seq = seqlist.begin(); seq != seqlist.end();
       seq++) {
    // skip old sequences not owned by us
    if (seq->owner != vbp.username && seq->modtime > 60 * 60 * 24 * 2 &&
        flags.count("all") < 1) {
      skippedcnt++;
      continue;
    }
    // if we're only doing one, check if this is it
    if (myseq > 0 && myseq != seq->seqnum) continue;
    if (i++ % 20 == 0) {
      printf(
          "--------------------------------------------------------------------"
          "----\n");
      printf("Sequence Name        Num  Pri Owner    ");
      printf("Wait  Run   Bad   Done Total\n");
      printf(
          "--------------------------------------------------------------------"
          "----\n");
    }
    // M means the sequence has been moved and will soon be removed
    if (seq->status == 'M') continue;

    if (seq->name.size())
      printf("%-20.20s ", seq->name.c_str());
    else
      printf("%-20.20s ", "<unnamed sequence>");
    printf("%-5d", seq->seqnum);
    printf(" %-3d", seq->priority.priority);
    printf("%-10.10s", seq->owner.c_str());
    printf("%-6d", seq->waitcnt);
    printf("%-6d", seq->runcnt);
    printf("%-6d", seq->badcnt);
    printf("%-5d", seq->donecnt);
    printf("%-6d", seq->jobcnt);
    runcnt += seq->runcnt;
    badcnt += seq->badcnt;
    waitcnt += seq->waitcnt;
    donecnt += seq->donecnt;
    jobcnt += seq->jobcnt;
    seqcnt++;
    if (seq->status == 'K') {
      printf("[*]");
      diecnt++;
    } else if (seq->status == 'P') {
      printf("[P]");
      postcnt++;
    }
    printf("\n");
    if (myseq > 0) {
      if (diecnt)
        ret = vb_killed;
      else if (seq->badcnt)
        ret = vb_bad;
      else if (postcnt)
        ret = vb_postponed;
      else if (seq->runcnt)
        ret = vb_running;
      else
        ret = vb_waiting;
    }
  }
  if (seqcnt > 1) {
    printf(
        "----------------------------------------------------------------------"
        "--\n");
    cout << format("%-20.20s                    %-6d%-6d%-6d%-5d%-6d\n") %
                "Totals" % waitcnt % runcnt % badcnt % donecnt % jobcnt;
  }
  if (seqcnt)
    printf(
        "----------------------------------------------------------------------"
        "--\n");
  else
    printf("No active jobs in the queue.\n");
  if (skippedcnt)
    printf(
        "[!] %d idle sequences were omitted (use voxq -ss to include them)\n",
        skippedcnt);
  if (diecnt) printf("[*] this sequence has been killed\n");
  if (postcnt) printf("[P] this sequence has been postponed\n");
  return ret;
}

void voxq_sequencedetail(tokenlist &args) {
  int seqnum, hrs, mins, secs, jobnum;
  if (args.size() != 2 && args.size() != 3) {
    printf("[E] voxq: wrong number of arguments\n");
    return;
  }

  jobnum = -1;
  seqnum = strtol(args[1]);
  if (args.size() > 1) jobnum = strtol(args[2]);

  // FIXME the below reads it directly from the queue -- bad!
  VBSequence seq(vbp, seqnum);
  if (!seq.valid) {
    printf("No such valid sequence %d.\n", seqnum);
    return;
  }

  printf("     Number: %d\n", seq.seqnum);
  printf("       Name: %s\n", seq.name.c_str());
  printf("     Status: %c ", seq.status);
  if (seq.status == 'R')
    printf("(ready to run)\n");
  else if (seq.status == 'P')
    printf("(postponed)\n");
  else if (seq.status == 'K')
    printf("(killed)\n");
  else
    printf("(mystery status)\n");
  // printf("   Priority: %d jobs at pri
  // %d\n",seq.priority.maxjobs,seq.priority.priority); if
  // (seq.priority.maxjobs)
  //   printf("  Priority2: %d jobs at pri
  //   %d\n",seq.priority.maxjobs2,seq.priority.priority2);
  // printf(" Maxperhost: %d\n",seq.priority.maxperhost);
  printf("   Priority: %s\n", ((string)seq.priority).c_str());
  printf("      Owner: %s\n", seq.owner.c_str());
  vbforeach(string fh, seq.forcedhosts) printf("Forced Host: %s", fh.c_str());
  printf("   jobcount: %d total\n", seq.jobcnt);

  if (args[0] == "xx") {
    for (SMI js = seq.specmap.begin(); js != seq.specmap.end(); js++) {
      printf("\n");
      printf("JOB %d - %s\n", js->second.jnum, js->second.name.c_str());
      printf("logfile: %s\n", js->second.logfilename().c_str());
      printf("  status: %c ", js->second.status);
      if (js->second.status == 'R')
        printf("(running)\n");
      else if (js->second.status == 'W')
        printf("(waiting to run)\n");
      else if (js->second.status == 'S')
        printf("(shipped)\n");
      else if (js->second.status == 'B')
        printf("(bad)\n");
      else if (js->second.status == 'D')
        printf("(done)\n");
      else
        printf("(mystery status)\n");
      printf("  type: %s\n", js->second.jobtype.c_str());

      if (js->second.startedtime) {
        GetElapsedTime(js->second.startedtime, time(NULL), hrs, mins, secs);
        printf(
            "  most recently started %d:%02d:%02d ago (h:m:s) ago on host %s\n",
            hrs, mins, secs, js->second.hostname.c_str());
        if (js->second.finishedtime) {
          GetElapsedTime(js->second.startedtime, js->second.finishedtime, hrs,
                         mins, secs);
          printf("  finished +%d:%02d:%02d after being started", hrs, mins,
                 secs);
        }
        printf("\n");
      }
    }
  } else if (args[0] == "x") {
    set<int> rset, wset, sset, bset, dset, oset;
    for (SMI js = seq.specmap.begin(); js != seq.specmap.end(); js++) {
      if (js->second.status == 'W')
        wset.insert(js->first);
      else if (js->second.status == 'R')
        rset.insert(js->first);
      else if (js->second.status == 'S')
        sset.insert(js->first);
      else if (js->second.status == 'B')
        bset.insert(js->first);
      else if (js->second.status == 'D')
        dset.insert(js->first);
      else
        oset.insert(js->first);
    }
    cout << format("what are the jobs in this sequence doing?\n");
    if (rset.size())
      cout << format("these jobs are running: %s\n") % textnumberset(rset);
    if (wset.size())
      cout << format("these jobs are waiting: %s\n") % textnumberset(wset);
    if (sset.size())
      cout << format("these jobs are shipped: %s\n") % textnumberset(sset);
    if (bset.size())
      cout << format("these jobs are bad: %s\n") % textnumberset(bset);
    if (dset.size())
      cout << format("these jobs are done: %s\n") % textnumberset(dset);
    if (oset.size())
      cout << format("these jobs have unknown status: %s\n") %
                  textnumberset(oset);
  } else if (args[0] == "ls") {
    for (SMI js = seq.specmap.begin(); js != seq.specmap.end(); js++) {
      printf("JOB %d (%s) [%c]", js->second.jnum, js->second.jobtype.c_str(),
             js->second.status);
      if (js->second.startedtime) {
        GetElapsedTime(js->second.startedtime, time(NULL), hrs, mins, secs);
        printf("  most recently started %d:%02d:%02d (h:m:s) ago on host %s\n",
               hrs, mins, secs, js->second.hostname.c_str());
        if (js->second.finishedtime) {
          GetElapsedTime(js->second.startedtime, js->second.finishedtime, hrs,
                         mins, secs);
          printf("  finished +%d:%02d:%02d after being started", hrs, mins,
                 secs);
        }
        printf("\n");
      } else
        printf("\n");
    }
  }
}

void voxq_debug(tokenlist &args) {
  int seqnum, hrs, mins, secs, jobnum;
  if (args.size() != 2 && args.size() != 3) return;

  jobnum = -1;
  seqnum = strtol(args[1]);
  if (args.size() > 1) jobnum = strtol(args[2]);

  VBSequence seq(vbp, seqnum);
  if (!seq.valid) {
    printf("No such valid sequence %d.\n", seqnum);
    return;
  }

  printf("Sequence name: %s\n", seq.name.c_str());
  printf("       Status: %c\n", seq.status);
  printf("     Priority: %d\n", seq.priority.priority);
  printf("        Owner: %s\n", seq.owner.c_str());
  printf("   Total jobs: %d\n", seq.jobcnt);
  printf("     Bad jobs: %d\n", seq.badcnt);

  for (SMI js = seq.specmap.begin(); js != seq.specmap.end(); js++) {
    if (js->second.status != 'B') continue;
    printf("\n");
    printf("JOB %d - %s\n", js->second.jnum, js->second.name.c_str());
    printf("   logfile: %s\n", js->second.logfilename().c_str());
    printf("      type: %s\n", js->second.jobtype.c_str());
    pair<string, string> pp;
    vbforeach(pp, js->second.arguments)
        printf("  argument: %s=%s\n", pp.first.c_str(), pp.second.c_str());

    if (js->second.startedtime) {
      GetElapsedTime(js->second.startedtime, time(NULL), hrs, mins, secs);
      printf("  most recently started %d:%02d:%02d (h:m:s) ago on host %s\n",
             hrs, mins, secs, js->second.hostname.c_str());
      if (js->second.finishedtime) {
        GetElapsedTime(js->second.startedtime, js->second.finishedtime, hrs,
                       mins, secs);
        printf("  finished +%d:%02d:%02d after being started\n", hrs, mins,
               secs);
      }
      printf("\n");
    }
    // offer options for how to handle this job
    string resp =
        vb_tolower(vb_getchar("[v]iew log, [e]dit log, [s]kip, or [q]uit: "));
    printf("\n\n");
    if (resp == "v") {
      printf("======================BEGIN LOG FILE======================\n");
      string logfile = vbp.queuedir + "/" + js->second.seqdirname() + "/" +
                       js->second.basename() + ".log";
      system(str(format("cat %s") % logfile).c_str());
      printf("=======================END LOG FILE=======================\n");
    } else if (resp == "e") {
      if (fork() == 0) {
        string editor;
        editor = getenv("VISUAL");
        if (!editor.size()) editor = getenv("EDITOR");
        if (!editor.size()) editor = "emacs";
        system(
            (str(format("%s %s") % editor % js->second.logfilename()).c_str()));
      }
    } else if (resp == "s")
      continue;
    else if (resp == "q")
      break;
  }
}

void voxq_whynot(tokenlist &) {
  printf("[E] voxq: whynot is temporarily out of commission\n");
}

void voxq_marksequence(tokenlist &args, string status) {
  int ret;
  set<int32> seqnums, success, errors, commerrs;
  string returnmsg;

  for (size_t i = 1; i < args.size(); i++) add_numbers(seqnums, args[i]);

  vbforeach(int32 snum, seqnums) {
    ret = voxq_tell_scheduler(
        (format("setseqinfo %d status %s") % snum % status).str(), returnmsg);
    if (ret)
      commerrs.insert(snum);
    else if (returnmsg == "success")
      success.insert(snum);
    else if (returnmsg == "perm")
      errors.insert(snum);
    else
      errors.insert(snum);
  }
  if (success.size())
    cout << format("[I] the following sequences were updated: %s\n") %
                textnumberset(success);
  if (errors.size())
    cout << format("[I] the following sequences were not updated: %s\n") %
                textnumberset(errors);
  if (commerrs.size())
    cout << format(
                "[I] could not reach the scheduler to update the following "
                "sequences: %s\n") %
                textnumberset(commerrs);
}

void voxq_killsequence(tokenlist &args, string newstatus) {
  int ret;
  set<int32> seqnums, success, errors, commerrs;
  string returnmsg;

  for (size_t i = 1; i < args.size(); i++) add_numbers(seqnums, args[i]);

  vbforeach(int32 snum, seqnums) {
    ret = voxq_tell_scheduler("killsequence " + strnum(snum) + " " + newstatus,
                              returnmsg);
    if (ret)
      commerrs.insert(snum);
    else if (returnmsg == "success")
      success.insert(snum);
    else if (returnmsg == "perm")
      errors.insert(snum);
    else
      errors.insert(snum);
  }
  if (success.size())
    cout << format("[I] the following sequences were halted: %s\n") %
                textnumberset(success);
  if (errors.size())
    cout << format("[I] the following sequences were not halted: %s\n") %
                textnumberset(errors);
  if (commerrs.size())
    cout << format(
                "[I] could not reach the scheduler to halt the following "
                "sequences: %s\n") %
                textnumberset(commerrs);
}

// voxq -sched <sequencenumber(s)> ...
// see hlp file for more details

void voxq_sched(tokenlist &args) {
  int ret;
  string returnmsg, tmp;
  VBpri pp;

  // error
  if (args.size() < 2) {
    printf(
        "usage: voxq -sched <sequencenumbers> <maxjobs> <pri> <max2> <pri2> "
        "<maxperhost>\n");
    printf("   or:      -sched <sequencenumbers> <pri>\n");
    printf("   or:      -sched <sequencenumbers> <pri> <maxjobs>\n");
    printf("   or:      -m <maxjobs> <sequencenumbers>\n");
    printf("   or:      -# <sequencenumbers>      [where # is from 0 to 5]\n");
    printf(" note: priority 0 jobs will never run\n");
    printf(" note: maxjobs of 0 means no limit\n");
    printf(" note: when maxjobs is reached, pri2 is used (it can be 0)\n");
    printf(" note: sequence numbers can include ranges and comma-separated\n");
    printf("       lists (e.g., 1-5,7-10), but no spaces\n");
    return;
  }

  // -sched 10101 [interactive]
  if (args.size() == 2 && args[0] == "sched") {
    string tmps;
    cout << "(below, use 0 for no limit)\n";
    cout << format("max jobs per host [default=%d]: ") % pp.maxperhost;
    cin >> tmps;
    if (!tmps.empty()) pp.maxperhost = strtol(tmps);

    cout << format("max jobs at default priority [default=%d]: ") % pp.maxjobs;
    cin >> tmps;
    if (!tmps.empty()) pp.maxjobs = strtol(tmps);

    cout << format("default priority [default=%d]: ") % pp.priority;
    cin >> tmps;
    if (!tmps.empty()) pp.priority = strtol(tmps);

    if (pp.maxjobs > 0) {
      cout << format("max jobs at secondary priority [default=%d]: ") %
                  pp.maxjobs2;
      cin >> tmps;
      if (!tmps.empty()) pp.maxjobs2 = strtol(tmps);

      cout << format("secondary priority [default=%d]: ") % pp.priority2;
      cin >> tmps;
      if (!tmps.empty()) pp.priority2 = strtol(tmps);
    }
    tmps = str(format("setsched %s %s %d %d %d %d %d") % args[0] % args[1] %
               pp.maxjobs % pp.priority % pp.maxjobs2 % pp.priority2 %
               pp.maxperhost);

    ret = voxq_tell_scheduler(tmps, returnmsg);
    if (ret)
      printf("[E] voxq: couldn't communicate with scheduler (%d)\n", ret);
    else
      cout << returnmsg << endl;
    return;
  } else if (args.size() == 2 && args[0].size() == 1 &&
             strchr("012345", args[0][0])) {
    tmp = "setsched sched " + args[1] + " " + args[0];
    ret = voxq_tell_scheduler(tmp, returnmsg);
    if (ret)
      printf("[E] voxq: couldn't communicate with scheduler (%d)\n", ret);
    else
      cout << returnmsg << endl;
    return;
  } else if (args.size() > 2) {
    tmp = "setsched " + args.Tail(0);
    ret = voxq_tell_scheduler(tmp, returnmsg);
    if (ret)
      printf("[E] voxq: couldn't communicate with scheduler (%d)\n", ret);
    else
      cout << returnmsg << endl;
    return;
  } else {
    printf(
        "usage: voxq -sched <sequencenumbers> <maxjobs> <pri> <max2> <pri2> "
        "<maxperhost>\n");
    printf("   or:      -sched <sequencenumbers> <pri>\n");
    printf("   or:      -sched <sequencenumbers> <pri> <maxjobs>\n");
    printf(" note: priority 0 jobs will never run\n");
    printf(" note: maxjobs of 0 means no limit\n");
    printf(" note: when maxjobs is reached, pri2 is used (it can be 0)\n");
    printf(" note: sequence numbers can include ranges and comma-separated\n");
    printf("       lists (e.g., 1-5,7-10), but no spaces\n");
    return;
  }
  return;
}

void voxq_changejobs(tokenlist &args, char from, char to) {
  int ret;
  char tostring[2];
  set<int32> seqnums;
  char tmp[STRINGLEN];
  string returnmsg;

  for (size_t i = 1; i < args.size(); i++) add_numbers(seqnums, args[i]);

  tostring[0] = to;
  tostring[1] = 0;
  vbforeach(int32 snum, seqnums) {
    sprintf(tmp, "changejobstatus %d %c %c", snum, from, to);
    ret = voxq_tell_scheduler(tmp, returnmsg);
    if (!ret)
      printf("[I] %s\n", returnmsg.c_str());
    else
      printf("[E] voxq: couldn't communicate with scheduler (%d)\n", ret);
  }
}

void voxq_gimme(tokenlist &args, int gimflag) {
  char tmp[STRINGLEN];
  int hours;
  string returnmsg;

  if (gimflag && (args.size() < 2 || args.size() > 3)) {
    printf("[E] voxq: usage: voxq -g <hostname> [max number of hours]\n");
    return;
  } else if (!gimflag && args.size() != 2) {
    printf("[E] voxq: usage: voxq -b <hostname>\n");
    return;
  }

  if (args.size() == 3)
    hours = strtol(args[1]);
  else
    hours = 1;

  if (gimflag)
    sprintf(tmp, "GIMME %s %d", args[1].c_str(), hours);
  else
    sprintf(tmp, "GIVEBACK %s", args[1].c_str());

  int err = voxq_tell_scheduler(tmp, returnmsg);
  if (!err)
    printf("%s\n", returnmsg.c_str());
  else
    printf("Couldn't communicate with scheduler (%d).\n", err);
}

void voxq_die() {
  int err;
  string returnmsg;

  if (!vbp.su) {
    printf("Sorry, only a VoxBo superuser can kill the scheduler.\n");
    return;
  }
  err = voxq_tell_scheduler("DIE", returnmsg);
  switch (err) {
    case 0:
      printf("The scheduler has shut down.\n");
      break;
    case 1:
      printf("Sorry, the scheduler does not appear to be running.\n");
      break;
    case 2:
      printf("Sorry, we couldn't find the scheduler's IP address.\n");
      break;
    case 3:
      printf("Sorry, we couldn't send a message to the scheduler.\n");
      break;
    default:
      printf("Sorry, unexpected error communicating with the scheduler.\n");
      break;
  }
}

void voxq_reset() {
  int err;
  string returnmsg;

  if (!vbp.su) {
    printf("Sorry, only a VoxBo superuser can kill the scheduler.\n");
    return;
  }
  err = voxq_tell_scheduler("RESET", returnmsg);
  switch (err) {
    case 0:
      printf("The scheduler has been asked to reset itself.\n");
      break;
    case 1:
      printf("Sorry, we couldn't figure out where the scheduler is running.\n");
      break;
    case 2:
      printf("Sorry, we couldn't find the scheduler's IP address.\n");
      break;
    case 3:
      printf("Sorry, we couldn't send the RESET message to the scheduler.\n");
      break;
    default:
      printf("Sorry, unexpected error communicating with the scheduler.\n");
      break;
  }
}

void voxq_server(tokenlist &args) {
  if (!vbp.su) {
    printf(
        "[E] voxq: sorry, you must be a VoxBo superuser to use this command\n");
    return;
  }
  string cmd = vb_tolower(args[1]);
  string rmsg;
  int err;
  if (cmd == "add" && args.size() == 3) {
    err = voxq_tell_scheduler("ADDSERVER " + args[2], rmsg);
    printf("%s\n", rmsg.c_str());
  } else if ((cmd == "remove" || cmd == "delete") && args.size() == 3) {
    err = voxq_tell_scheduler("DELSERVER " + args[2], rmsg);
    printf("%s\n", rmsg.c_str());
  } else {
    printf("[E] voxq: --server usage:\n");
    printf("[E]       voxq --server add <server>\n");
    printf("[E]       voxq --server remove <server>\n");
    return;
  }
}

void voxq_queue(tokenlist &args) {
  if (!vbp.su) {
    printf(
        "[E] voxq: sorry, you must be a VoxBo superuser to use this command\n");
    return;
  }
  string cmd = vb_tolower(args[1]);
  string rmsg;
  int err;
  if (cmd == "on") {
    err = voxq_tell_scheduler("QUEUEON", rmsg);
  } else if (cmd == "off")
    err = voxq_tell_scheduler("QUEUEOFF", rmsg);
  else {
    printf("[E] voxq: unrecognized command %s\n", cmd.c_str());
    return;
  }
  if (err) {
    printf("[E] voxq: error contacting scheduler\n");
    return;
  }
  printf("%s\n", rmsg.c_str());
}

int voxq_send_file(tokenlist &args) {
  char tmp[STRINGLEN];
  int s, err;

  struct sockaddr_in addr;
  if ((err = get_addr_inet(addr))) return err;
  if ((s = safe_connect(&addr, 2.0)) < 0) return 103;
  // send authentication string
  sprintf(tmp, "%s", vbp.username.c_str());
  if (send(s, tmp, strlen(tmp) + 1, 0) < (int)strlen(tmp) + 1) {
    close(s);
    return 4;
  }
  // receive ACK
  int cnt = safe_recv(s, tmp, STRINGLEN, 10.0);
  if (cnt <= 3) {
    close(s);
    return 4;
  }
  // send indication we're sending
  if (send(s, "SENDFILE", 9, 0) < 9) {
    close(s);
    return 4;
  }
  // send file
  if (send_file(s, args[1])) {
    close(s);
    return 4;
  }
  // receive ACK
  cnt = safe_recv(s, tmp, STRINGLEN, 10.0);
  if (cnt <= 0) tmp[0] = '\0';
  tmp[4] = '\0';
  close(s);
  return 0;
}

int get_hostlist() {
  if (hostlist.size()) return 0;
  string hostinfo;
  VBHost tmph;
  if (get_scheduler_info("HOSTS", hostinfo)) return 101;
  // cout << hostinfo << endl;
  tokenlist args;
  args.SetSeparator("\n");
  args.SetQuoteChars("");
  args.ParseLine(hostinfo);
  // args.print();
  for (size_t i = 0; i < args.size(); i++) {
    VBHost tmph;
    if (!(tmph.frombuffer(args[i]))) hostlist.push_back(tmph);
  }
  hostlist.sort(cmp_host_name);
  return 0;
}

int get_sequencelist() {
  if (seqlist.size()) return 0;
  string hostinfo, field;
  tokenlist args, argx;
  VBSequence tmps;
  if (get_scheduler_info("SEQUENCES", hostinfo)) return 101;
  args.SetQuoteChars("[<(\"'");
  argx.SetQuoteChars("[<(\"'");
  args.ParseLine(hostinfo);
  for (size_t i = 0; i < args.size(); i++) {
    argx.ParseLine(args[i]);
    field = argx[0];
    if (field == "name")
      tmps.name = argx[1];
    else if (field == "num")
      tmps.seqnum = strtol(argx[1]);
    else if (field == "priority")
      tmps.priority = strtol(argx[1]);
    else if (field == "status")
      tmps.status = argx[1][0];
    else if (field == "owner")
      tmps.owner = argx[1];
    else if (field == "idle")
      tmps.modtime = strtol(argx[1]);
    else if (field == "counts") {
      tmps.jobcnt = strtol(argx[1]);
      tmps.waitcnt = strtol(argx[2]);
      tmps.runcnt = strtol(argx[3]);
      tmps.badcnt = strtol(argx[4]);
      tmps.donecnt = strtol(argx[5]);
    } else if (field == "EOS") {
      seqlist.push_back(tmps);
      tmps.init();
    }
  }
  return 0;
}

int get_scheduler_info(string msg, string &info) {
  const int BUFSIZE = 32768;
  char tmp[BUFSIZE];
  int s, err;
  info = "";

  struct sockaddr_in addr;
  if ((err = get_addr_inet(addr))) return err;
  if ((s = safe_connect(&addr, 2.0)) < 0) return 103;
  // send authentication string
  sprintf(tmp, "%s", vbp.username.c_str());
  if (send(s, tmp, strlen(tmp) + 1, 0) < (int)strlen(tmp) + 1) {
    close(s);
    return 4;
  }
  // receive ACK
  int cnt = safe_recv(s, tmp, BUFSIZE, 10);
  if (cnt <= 3) {
    close(s);
    return 4;
  }
  // send message
  if (send(s, msg.c_str(), msg.size() + 1, 0) < (int)msg.size() + 1) {
    close(s);
    return 4;
  }
  while (1) {
    // receive line of info
    cnt = safe_recv(s, tmp, BUFSIZE, 10);
    if (cnt <= 0) break;
    info += tmp;
  }
  close(s);
  return 0;
}

void voxq_dumpconfig(tokenlist &args) {
  if (!vbp.su) {
    printf("Sorry, this option is only available to privileged parties.\n");
    return;
  }

  printf("\nVoxBo configuration info:\n\n");

  if (vbp.su)
    printf("SUPER USER, ");
  else
    printf("REGULAR USER, ");

  printf("\n\n");

  // user stuff
  printf("User Config:\n");
  printf("  username: %s\n", vbp.username.c_str());
  printf("  email address: %s\n", vbp.email.c_str());
  printf("  home directory: %s\n", vbp.homedir.c_str());
  printf("  user directory: %s\n", vbp.userdir.c_str());

  // system stuff
  printf("\n");
  printf("System Config:\n");
  printf("  sendmail: %s\n", vbp.sendmail.c_str());
  printf("queuedir: %s\n", vbp.queuedir.c_str());
  printf("  root directory %s\n", vbp.rootdir.c_str());
  printf("  host name: %s\n", vbp.thishost.hostname.c_str());
  printf("  short hostname: %s\n", vbp.thishost.nickname.c_str());
  printf("  sysadmin email: %s\n", vbp.sysadmin.c_str());
  printf("  queue interval: %d seconds\n", vbp.queuedelay);
  printf("  uid of voxbo owner: %d\n", (int)vbp.voxbouid);
  printf("\n");
  string returnmsg;
  voxq_tell_scheduler((string) "DIAG " + args[1], returnmsg);
}

void voxq_dumpjobtypes(tokenlist &args) {
  vbp.read_jobtypes();
  if (!vbp.su) {
    printf("[E] voxq: this option is only available to privileged parties.\n");
    return;
  }
  int longflag = 0;
  if (args.size() == 2) {
    string flag = vb_tolower(args[1]);
    if (flag == "-l") longflag = 1;
  }

  if (longflag == 0) {
    printf("All installed jobtypes: ");
    for (TI jt = vbp.jobtypemap.begin(); jt != vbp.jobtypemap.end(); jt++) {
      printf("%s ", jt->second.shortname.c_str());
    }
    printf("\n");
  } else {
    printf("\n");
    for (TI jt = vbp.jobtypemap.begin(); jt != vbp.jobtypemap.end(); jt++) {
      printf("\n");
      jt->second.print();
    }
    printf("\n");
  }
}

void voxq_cpus(string) {
  if (get_hostlist()) {
    printf("[E] voxq: couldn't retrieve list of servers\n");
    return;
  }
  if (hostlist.size() == 0) {
    printf("No servers currently available.\n");
    return;
  }

  // FIXME switch to strings and boost::format
  char tmp[STRINGLEN], hostinfo[STRINGLEN];
  char elapsed[STRINGLEN];
  int hrs, mins, secs;
  HI host;
  char jobdesc[STRINGLEN];

  printf("\nServer      Load Pri Job                            Pct Elapsed\n");

  // below: for each host, first get status info, then check on what's
  // running, then check on reservations.  only print the host, load,
  // etc.  on the first printed line

  for (host = hostlist.begin(); host != hostlist.end(); host++) {
    // only superusers see hosts that haven't responded yet
    if (host->status == "unknown" && !vbp.su) continue;
    snprintf(hostinfo, STRINGLEN, "%-11.11s %-3.2f  %1d  ",
             host->nickname.c_str(), host->LoadAverage(), host->currentpri);
    if (host->status == "down") {
      printf("%-11.11s          <down>\n", host->nickname.c_str());
      hostinfo[0] = 0;
    }
    if (host->status == "dead") {
      printf("%-11.11s          <dead>\n", host->nickname.c_str());
      hostinfo[0] = 0;
    }

    // now see what's running on this host, across all queues
    if (host->runninglist.size() == 0 && strlen(hostinfo) > 0)
      printf("%-20.20s <idle>\n", hostinfo);
    else {
      for (JI js = host->runninglist.begin(); js != host->runninglist.end();
           js++) {
        GetElapsedTime(0, js->startedtime, hrs, mins, secs);
        if (hrs || mins || secs)
          sprintf(elapsed, "%d:%02d:%02d", hrs, mins, secs);
        else
          sprintf(elapsed, "<notime>");
        if (js->name.size() > 0) {
          string newname;
          vbforeach(char c, js->name) {
            if (isprint(c)) newname += c;
          }
          sprintf(jobdesc, "%s (%05d-%05d)", newname.c_str(), js->snum,
                  js->jnum);
        } else
          sprintf(jobdesc, "[unnamed job] (%05d-%05d)", js->snum, js->jnum);
        if (js->percentdone > -1 && js->percentdone < 101)
          sprintf(tmp, "%d", js->percentdone);
        else
          sprintf(tmp, "-");
        printf("%-20.20s %-30.30s %3.3s %-8.8s\n", hostinfo, jobdesc, tmp,
               elapsed);
        hostinfo[0] = 0;  // print hostinfo only on first iteration
      }
    }
    // FIXME do something about gimmes here
    // if no jobs or gimmes, still print something!
  }
  printf("\n");
}

void voxq_resources() {
  if (get_hostlist()) return;

  HI host;
  tokenlist args;

  printf("\nServer         Load  CPU Usage Resources\n");

  for (host = hostlist.begin(); host != hostlist.end(); host++) {
    printf("%-14.14s %-3.2f   %d of %d   ", host->nickname.c_str(),
           host->LoadAverage(), host->taken_cpus, host->total_cpus);
    for (RI rr = host->resources.begin(); rr != host->resources.end(); rr++)
      printf("%s(%d) ", rr->second.name.c_str(), rr->second.cnt);
    printf("\n");
    if (host->status == "down") {
      printf("%s is down\n", host->nickname.c_str());
      continue;
    }
    if (host->status == "dead") {
      printf("%s is not responding\n", host->nickname.c_str());
      continue;
    }
  }
  printf("\n");
}

void server2host(string servername, char *hostname) {
  if (get_hostlist()) return;

  hostname[0] = 0;
  for (HI host = hostlist.begin(); host != hostlist.end(); host++) {
    if (dancmp(host->nickname.c_str(), servername.c_str()))
      strcpy(hostname, host->hostname.c_str());
  }
}

void add_numbers(set<int32> &nums, string str) {
  set<int32> newnums = numberset(str);
  vbforeach(int32 ii, newnums) nums.insert(ii);
}

void quit_handler(int) {
  printf("<aborted>\n");
  exit(5);
}

int get_addr_inet(struct sockaddr_in &addr) {
  struct sockaddr_in *xaddr = (struct sockaddr_in *)&(addr);
  struct hostent *hp;
  tokenlist args;

  args.ParseFirstLine(vbp.rootdir + "/etc/scheduler.pid");
  if (args.size() < 2) return 101;
  // set up the host address
  memset(&addr, 0, sizeof(struct sockaddr_in));
  xaddr->sin_family = AF_INET;
  xaddr->sin_port = htons(vbp.serverport + 1);
  hp = gethostbyname(args[0].c_str());
  if (!hp) return 102;
  memcpy(&xaddr->sin_addr, hp->h_addr_list[0], hp->h_length);
  return 0;
}

int get_addr_unix(struct sockaddr_un &addr) {
  // FIXME non-functional!
  return -1;
  struct sockaddr_un *xaddr = (struct sockaddr_un *)&(addr);
  tokenlist args;
  char sockname[1024];
  printf(sockname, "/tmp/vbsocket.%s", vbp.username.c_str());

  // set up the host address
  memset(&addr, 0, sizeof(struct sockaddr_un));
  xaddr->sun_family = AF_LOCAL;
  strcpy(xaddr->sun_path, sockname);
  return 0;
}

int voxq_tell_scheduler(string msg, string &returnmsg) {
  char tmp[STRINGLEN];
  int s, err;

  struct sockaddr_in addr;
  if ((err = get_addr_inet(addr))) return err;
  if ((s = safe_connect(&addr, 4.0)) < 0) return 103;
  // send authentication string
  sprintf(tmp, "%s", vbp.username.c_str());
  if (send(s, tmp, strlen(tmp) + 1, 0) < (int)strlen(tmp) + 1) {
    close(s);
    return 4;
  }
  // receive ACK
  int cnt = safe_recv(s, tmp, STRINGLEN, 10.0);
  if (cnt <= 3) {
    close(s);
    return 4;
  }

  // send message
  if (send(s, msg.c_str(), msg.size() + 1, 0) < (int)msg.size() + 1) {
    close(s);
    return 4;
  }
  // receive ACK
  cnt = safe_recv(s, tmp, STRINGLEN, 10.0);
  if (cnt <= 1) tmp[0] = '\0';
  returnmsg = tmp;
  close(s);
  return 0;
}

void voxq_help() {
  cout << boost::format(help1) % vbversion;
  if (vbp.su) cout << boost::format(help2);
  printf(
      "\n<num> can be a single sequence number, a range, or both (e.g., "
      "1-3,7-10)\n");
  printf("\n");
}

void voxq_version() { printf("VoxBo voxq (v%s)\n", vbversion.c_str()); }
