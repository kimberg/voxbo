
// vbbatch.cpp
// queue up jobs that execute a command multiple times with different files or
// dirs Copyright (c) 2004-2010 by The VoxBo Development Team

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

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fstream>
#include <iostream>
#include <list>
#include "vbbatch.hlp.h"
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbutil.h"
#include "vbx.h"

void vbbatch_help();
void vbbatch_version();

typedef list<string>::iterator LSI;

class BatchConfig {
 public:
  list<string> filelist;
  void Run(tokenlist &args);
  void CreateBatch();
  string command;
  string dirname;     // directory (for tmp files, tes and cov)
  string varstring;   // "FILE" by default
  string basestring;  // "BASE" by default
  string indstring;   // "IND" by default
  string logdir;
  bool validate;
  bool interactive;
  bool usex;
  bool emailflag;
  string sname, jname;
  map<string, int> reqs;  // requirements
  VBSequence seq;
  int Ask(string str);
  void ShowCommandLines();
};

VBPrefs vbp;

int main(int argc, char *argv[]) {
  tokenlist args;

  vbp.init();
  vbp.read_jobtypes();
  args.Transfer(argc - 1, argv + 1);
  if (args.size() == 0) {
    vbbatch_help();
    exit(0);
  }

  BatchConfig bc;
  bc.seq.source = "[" + xgetcwd() + "] " + xcmdline(argc, argv);
  bc.Run(args);
  exit(0);
}

void BatchConfig::Run(tokenlist &args) {
  dirname = "/tmp";
  emailflag = 1;
  interactive = 0;
  validate = 1;
  usex = 0;
  varstring = "FILE";
  basestring = "BASE";
  indstring = "IND";
  seq.init();
  seq.email = vbp.email;
  seq.priority.maxjobs = 10;
  seq.seqnum = 100;

  int f_run = vbp.cores;
  int flushflag = 0;
  int appendflag = 0;
  int submitflag = 0;
  usex = 0;
  string seqname, seqdir;
  sname = "vbbatch sequence";
  jname = "vbbatch job";
  struct stat st;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-c" && i < args.size() - 1)
      command = args[++i];
    else if (args[i] == "-p" && i < args.size() - 1) {
      seq.priority.set(args[++i]);
    } else if (args[i] == "--run") {
      f_run = ncores() - 1;
      if (f_run < 1) f_run = 1;
    } else if (args[i].compare(0, 6, "--run=") == 0) {
      f_run = strtol(args[i].substr(6));
      if (f_run < 1) f_run = 1;
    } else if (args[i] == "-r")
      validate = 0;
    else if (args[i] == "-xfile" && i < args.size() - 1)
      varstring = args[++i];
    else if (args[i] == "-xind" && i < args.size() - 1)
      indstring = args[++i];
    else if (args[i] == "-xbase" && i < args.size() - 1)
      basestring = args[++i];
    else if (args[i] == "-req" && i < args.size() - 1)
      reqs[args[++i]] = 0;
    else if (args[i] == "-l" && i < args.size() - 1) {
      logdir = args[++i];
      createfullpath(logdir);
    } else if (args[i] == "-e" && i < args.size() - 1) {
      seq.email = args[++i];
      if (seq.email == "--") emailflag = 0;
    } else if (args[i] == "-m" && i < args.size() - 1)
      seq.priority.maxjobs = strtol(args[++i]);
    else if (args[i] == "-f" && i < args.size() - 1) {
      flushflag = 1;
      seqname = args[++i];
    } else if (args[i] == "-a" && i < args.size() - 1) {
      emailflag = 0;
      appendflag = 1;
      seqname = args[++i];
    } else if (args[i] == "-sn" && i < args.size() - 1) {
      sname = args[++i];
    } else if (args[i] == "-jn" && i < args.size() - 1) {
      jname = args[++i];
    } else if (args[i] == "-s" && i < args.size() - 1) {
      submitflag = 1;
      seqname = args[++i];
    } else if (args[i] == "-i") {
      interactive = 1;
      validate = 0;
    } else if (args[i] == "-h") {
      vbbatch_help();
      exit(0);
    } else if (args[i] == "-g")
      usex = 1;
    else if (args[i] == "-v") {
      vbbatch_version();
      exit(0);
    } else if (args[i] == "-d" && i < args.size() - 1) {
      for (int j = 0; j < strtol(args[i + 1]); j++)
        filelist.push_back(strnum(j));
      i++;
    } else
      filelist.push_back(args[i]);
  }

  // name of sequence in queue
  seq.name = sname;

  // put the requirements in place
  seq.requires = reqs;

  if (filelist.size() == 0 && !submitflag && !flushflag) {
    fprintf(stderr, "[E] vbbatch: no files specified\n");
    exit(212);
  }

  if (seqname != "") seqdir = vbp.userdir + "/private/" + seqname;

  if (flushflag) {
    rmdir_force(seqdir);
    if (!(stat(seqdir.c_str(), &st))) {
      printf("[E] vbbatch: couldn't flush sequence %s\n", seqname.c_str());
      exit(5);
    }
    printf("[I] vbbatch: sequence %s has been flushed\n", seqname.c_str());
    exit(0);
  } else if (appendflag) {
    // if the sequence dir doesn't exist, write it out like a fresh sequence
    if (stat(seqdir.c_str(), &st)) {
      CreateBatch();
      mkdir(vbp.userdir.c_str(), 0777);
      mkdir((vbp.userdir + "/private").c_str(), 0777);
      if (seq.Write(seqdir)) {
        printf("[E] vbbatch: failed to create new sequence %s\n",
               seqname.c_str());
        exit(10);
      } else {
        printf("[I] vbbatch: created new sequence %s\n", seqname.c_str());
        exit(0);
      }
    }
    // sequence already exists, get the next job number, create batch,
    // and then write the individual jobs
    VBSequence oldseq(seqdir, -3);
    int highest = oldseq.specmap.rbegin()->first;
    CreateBatch();
    seq.renumber(highest + 1);
    char tmpname[STRINGLEN];
    for (SMI jj = seq.specmap.begin(); jj != seq.specmap.end(); jj++) {
      sprintf(tmpname, "%s/%05d.job", seqdir.c_str(), jj->first);
      if (jj->second.Write(tmpname)) {
        printf("[E] vbbatch: there was an error appending your new jobs\n");
        exit(10);
      }
    }
    printf("[I] vbbatch: %d jobs have been appended to sequence %s\n",
           (int)seq.specmap.size(), seqname.c_str());
    exit(0);
  } else if (submitflag) {
    VBSequence seq(seqdir);
    seq.name = seqname;
    if (seq.specmap.size() == 0) {
      printf("[E] vbbatch: invalid or empty sequence %s\n", seqname.c_str());
      exit(11);
    }
    // add a notify job if needed
    if (emailflag && seq.email.size()) {
      VBJobSpec js;
      js.init();
      js.jobtype = "notify";
      js.dirname = dirname;
      js.arguments["email"] = seq.email;
      js.arguments["msg"] = "Your commands have been run.";
      js.name = "Notify";
      js.logdir = logdir;
      js.jnum = 0;
      for (SMI jj = seq.specmap.begin(); jj != seq.specmap.end(); jj++) {
        js.waitfor.insert(jj->second.jnum);
        if (jj->second.jnum >= js.jnum) js.jnum = jj->second.jnum + 1;
      }
      seq.specmap[js.jnum] = js;
    }
    if (f_run) {
      runseq(vbp, seq, f_run);
      // QRunSeq qr;
      // qr.Go(vbp,seq,f_run);
      // qr.exec();
    } else {
      vbreturn ret(0, "");
      if ((ret = seq.Submit(vbp))) {
        printf("[E] vbbatch: %s\n", ret.message().c_str());
        exit(11);
      }
      printf("[I] vbbatch: %s\n", ret.message().c_str());
    }
    rmdir_force(seqdir);
    exit(0);
  }

  if (validate)
    ShowCommandLines();
  else {
    CreateBatch();
    if (f_run) {
      runseq(vbp, seq, f_run);
      // QRunSeq qr;
      // qr.Go(vbp,seq,f_run);
      // qr.exec();
    } else {
      vbreturn ret(0, "");
      if ((ret = seq.Submit(vbp))) {
        printf("[E] vbbatch: %s\n", ret.message().c_str());
        exit(11);
      }
      printf("[I] vbbatch: %s\n", ret.message().c_str());
    }
  }
}

void BatchConfig::ShowCommandLines() {
  string cline;
  char tmps[64];
  printf("[I] vbbatch: the following command lines were generated:\n");
  int i = 0;
  for (LSI ff = filelist.begin(); ff != filelist.end(); ff++) {
    cline = command;
    replace_string(cline, varstring, *ff);
    replace_string(cline, basestring, xsetextension(*ff, "", 1));
    sprintf(tmps, "%05d", i++);
    replace_string(cline, indstring, tmps);
    printf("  %s\n", cline.c_str());
  }
}

void BatchConfig::CreateBatch() {
  VBJobSpec js;
  int jobnum = 0;
  set<int32> jlist;
  string cline;
  string full_jname;
  char buf[1024];

  int i = -1;
  for (LSI ff = filelist.begin(); ff != filelist.end(); ff++) {
    i++;
    if (interactive) {
      if (!Ask(*ff)) continue;
    }
    js.init();
    js.name = "vbbatch cmd";
    js.jobtype = "shellcommand";
    js.dirname = xgetcwd();
    js.logdir = logdir;
    cline = command;
    full_jname = jname;
    replace_string(cline, varstring, *ff);
    replace_string(full_jname, varstring, *ff);
    replace_string(cline, basestring, xsetextension(*ff, "", 1));
    replace_string(full_jname, basestring, xsetextension(*ff, "", 1));
    sprintf(buf, "%05d", i);
    replace_string(cline, indstring, buf);
    replace_string(full_jname, indstring, buf);
    js.arguments["command"] = cline;
    js.name = full_jname;
    jlist.insert(jobnum);
    js.jnum = jobnum;
    jobnum++;
    seq.addJob(js);
  }
  // notify?
  if (emailflag && seq.email.size()) {
    js.init();
    js.jobtype = "notify";
    js.dirname = dirname;
    js.logdir = logdir;
    js.arguments["email"] = seq.email;
    js.arguments["msg"] = "Your commands have been run.";
    js.name = "Notify";
    js.waitfor = jlist;
    js.jnum = jobnum++;
    seq.addJob(js);
  }
}

int BatchConfig::Ask(string fname) {
  termios tsave, tnew;
  tcgetattr(0, &tsave);
  tcgetattr(0, &tnew);
  tnew.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSADRAIN, &tnew);
  string str;
  cout << "include " << fname << "? " << flush;
  str = cin.get();
  tcsetattr(0, TCSADRAIN, &tsave);
  if (str[0] == 'y' || str[0] == 'Y') {
    cout << "yes" << endl;
    return 1;
  } else {
    cout << "no" << endl;
    return 0;
  }
}

void vbbatch_help() { cout << boost::format(myhelp) % vbversion; }

void vbbatch_version() { printf("VoxBo vbbatch (v%s)\n", vbversion.c_str()); }
