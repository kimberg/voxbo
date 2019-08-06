
// vbprep.cpp
// executable that reads script files and creates jobs
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Tom King based on code by Daniel Y. Kimberg

#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbprep.hlp.h"
#include "vbpreplib.h"
#include "vbutil.h"
#include "vbx.h"

void vbprep_help();
void vbprep_version();
void vbprep_sample(FILE *);
void vbprep_make_sample(const string &fname);
void vbprep_show_sample();

using namespace std;

VBPrefs vbp;

int main(int argc, char **argv) {
  vbp.init();
  vbp.read_jobtypes();
  int err = 0;
  VBpri mypri;
  bool separateflag = 0;
  VBSequence bigseq;
  string selectedSequence;
  tokenlist fileName;
  string cmdline = xcmdline(argc, argv);
  tokenlist args;
  int f_run = vbp.cores;

  args.Transfer(argc - 1, argv + 1);
  if (args.size() == 0) {
    vbprep_help();
    exit(0);
  }

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h" || args[i] == "--help") {
      vbprep_help();
      exit(0);
    } else if (args[i] == "-v" || args[i] == "--version") {
      vbprep_version();
      exit(0);
    } else if (args[i] == "-p" && i < args.size() - 1)
      mypri.set(args[++i]);
    else if (args[i] == "-s")
      separateflag = 1;
    else if (args[i] == "-n" && i < args.size() - 1)
      selectedSequence = args[++i];
    else if (args[i] == "-x" && i < args.size() - 1) {
      vbprep_make_sample(args[++i]);
      exit(0);
    } else if (args[i] == "--run") {
      f_run = ncores() - 1;
      if (f_run < 1) f_run = 1;
    } else if (args[i].compare(0, 6, "--run=") == 0) {
      f_run = strtol(args[i].substr(6));
      if (f_run < 1) f_run = 1;
    } else
      fileName.Add(args[i]);
  }

  stringstream tmps;
  VBJobSpec js;
  int found = 0;
  vbreturn ret(0, "");

  VBPData prep(vbp);  // code for this class found in libvoxbo: vbpreplib.cpp.
                      // vbpreplib.h

  for (int fileNum = 0; fileNum < (int)fileName.size(); fileNum++) {
    // clear object in case more than one file is run
    prep.Clear();
    // get requested jobs
    if (prep.StoreDataFromFile(fileName[fileNum], selectedSequence) == 0) {
      printErrorMsg(
          VB_ERROR,
          "vbprep: Sequence indicated with -n flag not found in files.\n");
      exit(109);
    }
    for (unsigned int dataSetNum = 0; dataSetNum < prep.data.size();
         dataSetNum++) {
      // check that the jobtypes are valid before scheduling
      if (vbp.jobtypemap.size() < 1) {
        printErrorMsg(
            VB_ERROR,
            "vbprep: The list of job types did not have any elements.\n");
        exit(106);
      } else {
        for (unsigned int jobtypeNum = 0;
             jobtypeNum < prep.data[dataSetNum].joblist.size(); jobtypeNum++) {
          string myjobtype =
              vb_tolower(prep.data[dataSetNum].joblist[jobtypeNum].jobtype);
          for (TI it = vbp.jobtypemap.begin(); it != vbp.jobtypemap.end();
               it++) {
            if (myjobtype == vb_tolower(it->second.shortname)) {
              found = 1;
              break;
            }
          }
          // invalid job type
          if ((found == 0) && (myjobtype != "waitprev")) {
            printf("[E] vbprep: jobtype %s not found\n", myjobtype.c_str());
            exit(107);
          }
          found = 0;
        }
        // must use only commands of last joblist
        prep.data[dataSetNum].joblist = prep.study.joblist;
        // build job sequence
        err = prep.data[dataSetNum].BuildJobs(vbp);
        switch (err) {
          case 100:
            printErrorMsg(VB_ERROR,
                          "vbprep: There were no jobs in the script file.");
            return 100;
          case 101:
            printErrorMsg(
                VB_ERROR,
                "vbprep: waitprev cannot be the first command in script.");
            return 101;
          case 102:
            printErrorMsg(
                VB_ERROR,
                "vbprep: waitprev cannot be the last command in script.");
            return 102;
          case 103:
            printErrorMsg(
                VB_ERROR,
                "vbprep: A waitprev cannot follow another waitprev in script.");
            return 103;
          case 104:
            printErrorMsg(
                VB_ERROR,
                "vbprep: There were no files specified in the script.");
            return 104;
          case 105:
            printErrorMsg(
                VB_ERROR,
                "vbprep: A directory was not specified in the script.");
            return 105;
          case 106:
            printErrorMsg(
                VB_ERROR,
                "vbprep: One or more script files could not be opened.");
            return 106;
        }
      }
      // submit job sequence
      if (prep.data[dataSetNum].joblist.size() < 1) return -1;
      // run each unit separately?
      if (separateflag) {
        prep.data[dataSetNum].seq.priority = mypri;
        prep.data[dataSetNum].seq.source = "[" + xgetcwd() + "] " + cmdline;
        if (prep.data[dataSetNum].seq.specmap.size())
          createfullpath(
              prep.data[dataSetNum].seq.specmap.begin()->second.logdir);
        if (f_run) {
          runseq(vbp, prep.data[dataSetNum].seq, f_run);
        } else {
          ret = prep.data[dataSetNum].seq.Submit(vbp);
          if (ret) {
            printf("[E] vbprep: %s\n", ret.message().c_str());
            exit(111);
          } else
            printf("[I] vbprep: %s\n", ret.message().c_str());
        }
      } else {
        // copy sequence for first job
        if (bigseq.specmap.size() < 1)
          bigseq = prep.data[dataSetNum].seq;
        else {
          int highest = bigseq.specmap.rbegin()->second.jnum;
          prep.data[dataSetNum].seq.renumber(highest + 1);
          for (SMI jj = prep.data[dataSetNum].seq.specmap.begin();
               jj != prep.data[dataSetNum].seq.specmap.end(); jj++) {
            bigseq.specmap[jj->second.jnum] = jj->second;
          }
        }
      }
    }
  }
  if (!separateflag) {
    bigseq.priority = mypri;
    bigseq.source = "[" + xgetcwd() + "] " + cmdline;
    if (bigseq.specmap.size())
      createfullpath(bigseq.specmap.begin()->second.logdir);
    if (f_run) {
      runseq(vbp, bigseq, f_run);
    } else {
      ret = bigseq.Submit(vbp);
    }
    if (ret) {
      printf("[E] vbprep: %s\n", ret.message().c_str());
      exit(111);
    } else
      printf("[I] vbprep: %s\n", ret.message().c_str());
  }
}

void vbprep_help() { cout << boost::format(myhelp) % vbversion; }

void vbprep_version() { cout << format("VoxBo vbprep (v%s)\n") % vbversion; }

void vbprep_make_sample(const string &fname) {
  printf("Creating sample script file in %s...", fname.c_str());
  fflush(stdout);
  FILE *fp = fopen(fname.c_str(), "w");
  if (fp) {
    vbprep_sample(fp);
    fclose(fp);
    printf("done.\n");
  } else {
    printf("failed.\n");
  }
}

void vbprep_show_sample() { cout << vbpsample; }

void vbprep_sample(FILE *fp) { fprintf(fp, vbpsample); }
