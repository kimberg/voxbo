
// vbmakerfx.cpp
// creates a random effects TES from a description file
// Copyright (c) 1998-2002 by The VoxBo Development Team

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
#include <fstream>
#include <iostream>
#include "vbio.h"
#include "vbjobspec.h"
#include "vbmakerfx.hlp.h"
#include "vbprefs.h"
#include "vbutil.h"

void vbmakerfx_help();
void vbmakerfx_sample(FILE *);
void vbmakerfx_make_sample(const string &fname);
void vbmakerfx_show_sample();
void buildcontrastvector(char *tmp, vector<int> cv);
const int BUFLEN = 1024;
bool ignorewarnings, validateonly;

class RFXSubject {
 public:
  string glmpath;
  string glmstem;
  vector<int> contrastvector;
  int scale;
  int affineonlyflag;
  double voxelsize[3];
  string normparams;
  string originfile;
  string outfile;
  RFXSubject();
  void init();
};

class RFXConfig {
 public:
  RFXConfig(string fname);
  int ParseFile(string filename);

  string seqname;       // name of the sequence
  string dirname;       // directory (for tmp files, tes and cov)
  string tesfile;       // full path of tes file to produce
  string coveragefile;  // full path of coverage map to produce
  double skernel[3];
  vector<RFXSubject> glmlist;  // list of all GLM dirs

  RFXSubject rfxg;
  RFXSubject rfxs;

  string email;  // email address
  int pri;
  bool auditflag, emailflag;
  VBSequence seq;
  void init();
  int RunCheck();
  int WriteJobs();
  void Validate();
  void CreateRFXJobs(int chk);
};

VBPrefs vbp;

int main(int argc, char *argv[]) {
  tokenlist args;

  vbp.init();
  args.Transfer(argc - 1, argv + 1);
  if (args.size() == 0) {
    vbmakerfx_help();
    exit(0);
  }
  if (args[0] == "-h") {
    vbmakerfx_show_sample();
    exit(0);
  }
  if (args[0] == "-x") {
    if (args.size() > 1)
      vbmakerfx_make_sample(args[1]);
    else
      printf("vbmakerfx -x needs a filename to store the sample rfx file\n");
    exit(0);
  }

  // okay, let's fly
  tokenlist files;

  ignorewarnings = FALSE;
  validateonly = FALSE;

  for (int i = 0; i < args.size(); i++) {
    if (args[i] == "-i") {
      ignorewarnings = TRUE;
      continue;
    }
    if (args[i] == "-v") {
      validateonly = TRUE;
      continue;
    }
    RFXConfig *rfxc = new RFXConfig(args[i]);
    rfxc->CreateRFXJobs(i);
    rfxc->WriteJobs();
    delete rfxc;
  }
  exit(0);
}

void RFXConfig::init() {
  seqname = "";
  dirname = "";
  glmlist.clear();
  email = vbp.email;
  pri = 2;
  auditflag = 1;
  emailflag = 1;
  skernel[0] = skernel[1] = skernel[2] = 0.0;
  seq.init();
}

RFXSubject::RFXSubject() { init(); }

void RFXSubject::init() {
  glmpath = "";
  glmstem = "";
  contrastvector.clear();
  scale = 2;
  affineonlyflag = 0;
  voxelsize[0] = 3.75;
  voxelsize[1] = 3.75;
  voxelsize[2] = 5.0;
  normparams = "";
  originfile = "";
  outfile = "";
}

RFXConfig::RFXConfig(string fname) {
  init();
  ParseFile(fname);
}

int RFXConfig::ParseFile(string filename) {
  ifstream infile;
  char buf[BUFLEN];
  tokenlist args;

  RFXSubject *rfxptr = &rfxg;

  infile.open(filename.c_str());
  if (!infile) {
    cout << "*** vbmakerfx: couldn't open " << filename << "." << endl;
    return 1;  // error!
  }
  while (infile.getline(buf, BUFLEN, '\n')) {
    args.ParseLine(buf);
    if (args.size()) {
      if (args[0][0] == '#' || args[0][0] == ';') continue;
      if (args[0] == "name" || args[0] == "seqname") {
        seqname = args.Tail();
      } else if (args[0] == "dirname") {
        if (args.size() == 2)
          dirname = args[1];
        else
          cout << "*** vbmakerfx: dirname takes exactly one argument" << endl;
      } else if (args[0] == "glmdir") {
        if (args.size() != 2) {
          cout << "*** vbmakerfx: glmdir takes exactly one argument" << endl;
          continue;
        }
        rfxs = rfxg;
        rfxptr = &rfxs;
        rfxptr->glmpath = args[1];
        rfxptr->glmstem = args[1] + (string) "/" + xfilename(args[1]);
      } else if (args[0] == "add") {
        glmlist.push_back(*rfxptr);
        rfxptr = &rfxg;
      } else if (args[0] == "tesfile") {
        if (args.size() == 2)
          tesfile = args[1];
        else
          cout << "*** vbmakerfx: tesfile takes exactly one argument" << endl;
      } else if (args[0] == "originfile") {
        if (args.size() == 2)
          rfxptr->originfile = args[1];
        else
          cout << "*** vbmakerfx: originfile takes exactly one argument"
               << endl;
      } else if (args[0] == "normparams") {
        if (args.size() == 2)
          rfxptr->normparams = args[1];
        else
          cout << "*** vbmakerfx: normparams takes exactly one argument"
               << endl;
      } else if (args[0] == "coveragemap") {
        if (args.size() == 2)
          coveragefile = args[1];
        else
          cout << "*** vbmakerfx: coveragemap takes exactly one argument"
               << endl;
      } else if (args[0] == "skernel") {
        if (args.size() != 2 && args.size() != 4)
          cout << "*** vbmakerfx: skernel takes 1 or 3 arguments" << endl;
        else {
          skernel[0] = skernel[1] = skernel[2] = strtod(args[1]);
          if (args.size() == 4) {
            skernel[1] = strtod(args[2]);
            skernel[2] = strtod(args[3]);
          }
        }
      } else if (args[0] == "contrastvector") {
        if (args.size() < 2)
          cout << "*** vbmakerfx: contrastvector needs at least one value"
               << endl;
        else {
          rfxptr->contrastvector.clear();
          for (int i = 1; i < args.size(); i++)
            rfxptr->contrastvector.push_back(strtol(args[i]));
        }
      } else if (args[0] == "affineonly") {
        if (args.size() == 2) {
          if (args[1] == "yes" || args[1] == "true" || args[1] == "1")
            rfxptr->affineonlyflag = 1;
        } else
          cout << "*** vbmakerfx: kernel takes exactly one argument" << endl;
      } else if (args[0] == "scale") {
        if (args.size() == 2) {
          args[1] = vb_tolower(args[1]);
          if (args[1] == "t")
            rfxptr->scale = 0;
          else if (args[1] == "percent")
            rfxptr->scale = 1;
          else if (args[1] == "beta")
            rfxptr->scale = 2;
          else if (args[1] == "f")
            rfxptr->scale = 3;
          else if (args[1] == "snr")
            rfxptr->scale = 5;
        } else
          cout << "*** vbmakerfx: scale takes exactly one argument" << endl;
      } else if (args[0] == "voxelsize") {
        if (args.size() == 4) {
          rfxptr->voxelsize[0] = strtod(args[1]);
          rfxptr->voxelsize[1] = strtod(args[2]);
          rfxptr->voxelsize[2] = strtod(args[3]);
        } else
          cout << "*** vbmakerfx: voxelsize takes three arguments" << endl;
      } else if (args[0] == "pri") {
        if (args.size() == 2)
          pri = strtol(args[1]);
        else
          cout << "*** vbmakerfx: pri takes exactly one argument" << endl;
      } else if (args[0] == "audit") {
        if (args.size() != 2)
          cout << "*** vbmakerfx: audit takes exactly one argument" << endl;
        if (args[1] == "yes" || args[1] == "1")
          auditflag = TRUE;
        else if (args[1] == "no" || args[1] == "0")
          auditflag = FALSE;
        else
          cout << "*** vbmakerfx: unrecognized value for audit flag (should be "
                  "yes/no)"
               << endl;
      } else if (args[0] == "email") {
        emailflag = TRUE;
        if (args.size() == 2) email = args[1];
      } else {
        cout << "*** vbmakerfx: unrecognized keyword " << args[0] << endl;
      }
    }
  }
  infile.close();
  return 0;  // no error!
}

void RFXConfig::CreateRFXJobs(int chk) {
  VBJobSpec js;
  char tmp[10240];
  int jobnum = 0, onum;
  int qtime = time(NULL) % 1000;
  vector<int> jlist;

  seq.name = seqname;

  // set filenames for each single subject
  for (int i = 0; i < (int)glmlist.size(); i++) {
    sprintf(tmp, "%ld-%d-%d-%d.cub", (long)getpid(), chk, qtime, i);
    glmlist[i].outfile = tmp;
  }

  // create copyorigin and single cub job for each subject
  for (int i = 0; i < (int)glmlist.size(); i++) {
    if (glmlist[i].originfile.size()) {
      js.init();
      js.jobtype = "rfxcopy";
      js.dirname = dirname;
      js.arguments.Add(glmlist[i].originfile);        // originfile
      js.arguments.Add(glmlist[i].glmstem + ".prm");  // glmstem
      js.name = "rfxcopy";
      js.jnum = jobnum++;
      onum = js.jnum;
      seq.speclist.push_back(js);
    }

    // build single rfx volume in native space, generate mask
    js.init();
    js.jobtype = "rfxsingle";
    js.dirname = dirname;
    js.arguments.Add("glmdir " + glmlist[i].glmstem);     // glmstem
    js.arguments.Add("params " + glmlist[i].normparams);  // normparams
    js.arguments.Add("outfile " + glmlist[i].outfile);    // path to cube file
    buildcontrastvector(tmp, glmlist[i].contrastvector);
    js.arguments.Add((string) "contrast " +
                     tmp);  // contrast vector e.g., [0,0,0]
    sprintf(tmp, "scale %d", glmlist[i].scale);
    js.arguments.Add(tmp);  // scale
    js.magnitude = 0;       // set it to something!
    js.name = "rfxsingle";
    if (glmlist[i].originfile.size())
      js.waitfor.push_back(onum);  // wait for origin job
    js.jnum = jobnum++;
    jlist.push_back(js.jnum);
    seq.speclist.push_back(js);

    // normalize map and mask if needed
    if (glmlist[i].normparams.size()) {
      // FIXME
      sprintf(tmp, "%.4f", glmlist[i].voxelsize[0]);
      js.arguments.Add(tmp);  // vox1
      sprintf(tmp, "%.4f", glmlist[i].voxelsize[1]);
      js.arguments.Add(tmp);  // vox2
      sprintf(tmp, "%.4f", glmlist[i].voxelsize[2]);
      js.arguments.Add(tmp);  // vox3
      sprintf(tmp, "%d", glmlist[i].affineonlyflag);
      js.arguments.Add(tmp);  // affineonly
    }
  }

  // rfxmerge
  // first build subject list for IDL
  string sublist = "[";
  for (int i = 0; i < (int)glmlist.size(); i++) {
    sublist += (string) "'" + glmlist[i].outfile + (string) "'";
    if (i < (int)glmlist.size() - 1) sublist += ",";
  }
  sublist += "]";
  // now build the actual merge job
  js.init();
  js.jobtype = "rfxmerge";
  js.dirname = dirname;
  js.name = "RFX Merge";
  js.arguments.Add(sublist);
  js.arguments.Add(tesfile);
  js.arguments.Add(coveragefile);
  if ((skernel[0] + skernel[1] + skernel[2]) > 0.0001)
    sprintf(tmp, "[%.4f,%.4f,%.4f]", skernel[0], skernel[1], skernel[2]);
  else
    sprintf(tmp, "0");
  js.arguments.Add(tmp);
  js.waitfor = jlist;
  js.jnum = jobnum++;
  seq.speclist.push_back(js);

  // notify?
  if (emailflag && email.size()) {
    js.init();
    js.jobtype = "notify";
    js.dirname = dirname;
    js.arguments.Add(email);
    js.arguments.Add("Your RFX volume has been built.");
    js.magnitude = 0;  // set it to something!
    js.name = "Notify";
    js.waitfor.push_back(jobnum - 1);
    js.jnum = jobnum;
    seq.speclist.push_back(js);
  }
}

int RFXConfig::WriteJobs() {
  // copy a few things to the sequence
  seq.priority = pri;
  seq.email = email;
  return seq.Submit();
}

void buildcontrastvector(char *tmp, vector<int> cv) {
  char tmp2[64];
  sprintf(tmp, "[");
  for (int i = 0; i < (int)cv.size() - 1; i++) {
    sprintf(tmp2, "%d,", cv[i]);
    strcat(tmp, tmp2);
  }
  sprintf(tmp2, "%d]", cv[cv.size() - 1]);
  strcat(tmp, tmp2);
}

void vbmakerfx_make_sample(const string &fname) {
  printf("Creating sample glm file in %s...", fname.c_str());
  fflush(stdout);
  FILE *fp = fopen(fname.c_str(), "w");
  if (fp) {
    vbmakerfx_sample(fp);
    fclose(fp);
    printf("done.\n");
  } else {
    printf("failed.\n");
  }
}

void vbmakerfx_show_sample() {
  printf("\nHere's what a valid .rfx file looks like, more or less:\n\n");

  vbmakerfx_sample(stdout);
}

void vbmakerfx_sample(FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp,
          "###############################################################\n");
  fprintf(fp, "# Sample .rfx file for VoxBo vbmakerfx.\n");
  fprintf(fp,
          "###############################################################\n");
  fprintf(fp, "\n");

  fprintf(fp, "# name for your sequence (so you can spot it in the queue)\n");
  fprintf(fp, "seqname my rfx sequence\n");
  fprintf(fp, "# where do you want to put your output files?\n");
  fprintf(fp, "dirname /data/myanalysis/GROUP\n");
  fprintf(fp, "# what's name would you like for your group TES file?\n");
  fprintf(fp, "tesfile group.tes\n");
  fprintf(fp, "# what name would you like for your coverage map?\n");
  fprintf(fp, "coveragemap coverage.cub\n");
  fprintf(fp, "# what priority (1 is low, 4 is high)\n");
  fprintf(fp, "pri 2\n");
  fprintf(fp,
          "# what smoothing kernel for your group TES file (comment out for no "
          "smoothing)\n");
  fprintf(fp, "skernel 2 2 1.5\n");
  fprintf(fp, "# what kind of statistic? (t/f/percent/beta/snr)?\n");
  fprintf(fp, "scale beta\n");
  fprintf(fp, "# what is your contrast vector?\n");
  fprintf(fp, "contrastvector 1 -1 1 -1\n");
  fprintf(fp, "# affine only normalization?\n");
  fprintf(fp, "affineonly 0\n");
  fprintf(fp, "# voxel size in mm for output TES\n");
  fprintf(fp, "voxelsize 3.75 3.75 5.0\n");
  fprintf(fp,
          "# email address for notification, defaults to your voxbo email\n");
  fprintf(fp, "# email nobody@nowhere.com\n");
  fprintf(fp, "\n");

  fprintf(
      fp,
      "##################################################################\n");
  fprintf(
      fp,
      "# Now create paragraphs of specific stuff for each individual GLM.\n");
  fprintf(fp, "# (you can also override scale and contrastvector here)\n");
  fprintf(
      fp,
      "##################################################################\n");
  fprintf(fp, "\n");

  fprintf(fp, "# For each glmdir section below:\n");
  fprintf(fp, "#   comment out originfile to leave origin unchanged\n");
  fprintf(fp, "#   comment out normparams to skip normalization\n");
  fprintf(fp, "#   comment out add line to omit this subject temporarily\n");
  fprintf(fp, "\n");

  fprintf(fp, "glmdir /data/study/Models/myglm\n");
  fprintf(fp, "originfile /data/study/data/sub1/Anatomy/EPI.cub\n");
  fprintf(fp, "normparams /data/study/data/sub1/Anatomy/NormParams.ref\n");
  fprintf(fp, "add\n");
  fprintf(fp, "\n");
}

void vbmakerfx_help() { cout << boost::format(myhelp) % vbversion; }
