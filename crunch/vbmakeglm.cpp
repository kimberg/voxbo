
// vbmakeglm.cpp
// hack to create voxbo glm's
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
// original version written by Dan Kimberg
// written to accomplish the same thing as some original IDL code by
// Geoff Aguirre

using namespace std;

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "vbutil.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbio.h"
#include "glmutil.h"
#include "vbx.h"
#include <sys/wait.h>

VBPrefs vbp;

const int BUFLEN=1024;
bool f_ignorewarnings,f_validateonly,f_setuponly;
int32 f_run;
string glmname;

class GLMConfig {
private:
  GLMParams global,local,*gp;
public:
  GLMConfig(string fname,double tr,double kr);
  int ParseFile(string filename);
};

void vbmakeglm_help();
void vbmakeglm_sample(FILE *);
void vbmakeglm_make_sample(const string &fname);
void vbmakeglm_show_sample();

int
main(int argc,char **argv)
{
  tokenlist args;
  vbp.init();
  vbp.read_jobtypes();
  f_run=vbp.cores;

  args.Transfer(argc-1,argv+1);
  if (args.size() == 0) {
    vbmakeglm_help();
    exit(0);
  }
  if (args[0]=="-h") {
    vbmakeglm_show_sample();
    exit(0);
  }
  if (args[0]=="-x") {
    if (args.size() > 1)
      vbmakeglm_make_sample(args[1]);
    else
      printf("vbmakeglm -x needs a filename to store the sample glm file\n");
    exit(0);
  }
  
  // okay, let's fly

  f_ignorewarnings=0;
  f_validateonly=0;
  f_setuponly=0;
  vector<string> filelist;
  double x_tr=0.0,x_kr=0.0;

  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-i") {
      f_ignorewarnings=1;
      continue;
    }
    else if (args[i]=="-v") {
      f_validateonly=1;
      continue;
    }
    else if (args[i]=="-s") {
      f_setuponly=1;
      continue;
    }
    else if (args[i]=="--run") {
      f_run=ncores()-1;
      if (f_run<1) f_run=1;
    }
    else if (args[i].compare(0,6,"--run=")==0) {
      f_run=strtol(args[i].substr(6));
      if (f_run<1) f_run=1;
    }
    else if (args[i]=="-n" && (i+1)<args.size()) {
      glmname=args[i+1];
      i++;
      continue;
    }
    else if (args[i]=="-tr" && (i+1)<args.size()) {
      x_tr=strtod(args[i+1]);
      i++;
      continue;
    }
    else if (args[i]=="-kr" && (i+1)<args.size()) {
      x_kr=strtod(args[i+1]);
      i++;
      continue;
    }
    else {
      filelist.push_back(args[i]);
    }
  }
  for (size_t i=0; i<filelist.size(); i++) {
    GLMConfig *glmc=new GLMConfig(filelist[i],x_tr,x_kr);
    delete glmc;
  }
  exit(0);
}

// this is the sole constructor, it chews on the passed arguments

GLMConfig::GLMConfig(string fname,double tr,double kr)
{
  local.init();
  global.init();
  gp=&global;
  global.glmfile=fname;
  global.TR=tr;
  global.kerneltr=kr;
  ParseFile(fname);
}

int
GLMConfig::ParseFile(string filename)
{
  ifstream infile;
  char buf[BUFLEN];
  tokenlist args;
  stringstream tmps;

  infile.open(filename.c_str());
  if (!infile) {
    tmps << "vbmakeglm: couldn't open " << filename;
    printErrorMsg(VB_ERROR,tmps.str());
    return 1;  // error!
  }
  while (infile.getline(buf,BUFLEN,'\n')) {
    args.ParseLine(buf);
    if (args.size()) {
      if (args[0][0]=='#' || args[0][0] == ';')
        continue;
      if (args[0]=="name" || args[0]=="glm") {
        local=global;  // start with a copy of the global
        gp=&local;
        gp->name=args.Tail();
      }
      else if (args[0]=="dirname") {
        if (args.size() == 2)
          gp->dirname=args[1];
        else {
          tmps.str("");
          tmps << "vbmakeglm: dirname takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="scan") {
        if (args.size() != 2) {
          tmps.str("");
          tmps << "vbmakeglm: scan takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
          continue;
        }
        gp->scanlist.push_back(args[1]);
      }
      else if (args[0]=="lows") {
        if (args.size() == 2)
          gp->lows=strtol(args[1]);
        else {
          tmps.str("");
          tmps << "vbmakeglm: lows takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="highs") {
        if (args.size() == 2)
          gp->highs=strtol(args[1]);
        else {
          tmps.str("");
          tmps << "vbmakeglm: highs takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="middles") {
        if (args.size() > 0)
          gp->middles=args[1];
      }
      else if (args[0]=="pieces") {
        if (args.size() == 2)
          gp->pieces=strtol(args[1]);
        else {
          printf("[E] vbmakeglm: pieces takes exactly one argument\n");
        }
      }
      else if (args[0]=="orderg") {
        if (args.size() == 2)
          gp->orderg=strtol(args[1]);
        else {
          tmps.str("");
          tmps << "vbmakeglm: orderg takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="refname") {
        if (args.size() == 2)
          gp->refname=args[1];
        else {
          printf("[E] vbmakeglm: reference takes exactly one argument\n");
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="kernel") {
        if (args.size() == 2)
          gp->kernelname=args[1];
        else if (args.size()==3) {
          gp->kernelname=args[1];
          gp->kerneltr=strtod(args[2]);
        }
        else {
          tmps.str("");
          tmps << "vbmakeglm: kernel takes one or two arguments";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="noisemodel") {
        if (args.size() == 2)
          gp->noisemodel=args[1];
        else {
          tmps.str("");
          tmps << "vbmakeglm: noisemodel takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="gmatrix") {
        if (args.size() == 2)
          gp->gmatrix=args[1];
        else {
          tmps.str("");
          tmps << "vbmakeglm: gmatrix takes exactly one argument"; 
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="makerandfxg") {
        gp->rfxgflag=1;
        if (args.size() > 1) {
          tmps.str("");
          tmps << "vbmakeglm: arguments to makerandfxg ignored";
          printErrorMsg(VB_WARNING,tmps.str());
        }
      }
      else if (args[0]=="tr") {
        if (args.size() == 2)
          gp->TR=strtol(args[1]);
        else {
          tmps.str("");
          tmps << "vbmakeglm: tr takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="pri") {
        if (args.size() == 2)
          gp->pri=strtol(args[1]);
        else {
          tmps.str("");
          tmps << "vbmakeglm: pri takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="audit") {
        if (args.size() != 2) {
          tmps.str("");
          tmps << "vbmakeglm: audit takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
        if (args[1]=="yes" || args[1]=="1")
          gp->auditflag=1;
        else if (args[1]=="no" || args[1]=="0")
          gp->auditflag=0;
        else {
          tmps.str("");
          tmps << "vbmakeglm: unrecognized value for audit flag (should be yes/no)";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="meannorm") {
        if (args.size() != 2) {
          tmps.str("");
          tmps << "vbmakeglm: meannorm takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
        if (args[1]=="yes" || args[1]=="1") {
          gp->meannorm=1;
          gp->meannormset=1;
        }
        else if (args[1]=="no" || args[1]=="0") {
          gp->meannorm=0;
          gp->meannormset=1;
        }
      }
      else if (args[0]=="driftcorrect") {
        if (args.size() != 2) {
          tmps.str("");
          tmps << "vbmakeglm: driftcorrect takes exactly one argument";
          printErrorMsg(VB_ERROR,tmps.str());
        }
        if (args[1]=="yes" || args[1]=="1") {
          gp->driftcorrect=1;
        }
        else if (args[1]=="no" || args[1]=="0") {
          gp->driftcorrect=0;
        }
        else {
          tmps.str("");
          tmps << "vbmakeglm: unrecognized value for driftcorrect flag (should be yes/no)";
          printErrorMsg(VB_ERROR,tmps.str());
        }
      }
      else if (args[0]=="email") {
        gp->emailflag=1;
        if (args.size() == 2)
          gp->email=args[1];
      }
      else if (args[0]=="go" || args[0]=="end") {
        if (!glmname.size() || glmname==gp->name) {
          gp->FixRelativePaths();
          gp->Validate(f_ignorewarnings);
          if (gp->valid && !f_validateonly) {
            if (!gp->CreateGLMDir()) {
              if (f_setuponly)
                ;
              else if (f_run) {
                gp->CreateGLMJobs2();
                runseq(vbp,gp->seq,f_run);
              }
              else {
                gp->CreateGLMJobs2();
                if (gp->seq.Submit(vbp))
                  cout << "[E] vbmakeglm: error submitting your glm sequence\n";
                else
                  cout << "[I] vbmakeglm: your glm sequence was submitted successfully\n";
              }
            }
          }
        }
        gp=&global;
      }
      else if (args[0]=="contrast") {
        gp->contrasts.push_back(args.Tail());
      }
      else if (args[0]=="endx" || args[0]=="noend") {
        gp=&global;
      }
      else if (args[0]=="include") {
        if (args.size() == 2)
          ParseFile(args[1]);
      }
      else {
        tmps.str("");
        tmps << "vbmakeglm: unrecognized keyword " << args[0];
        printErrorMsg(VB_ERROR,tmps.str());
      }
    }
  }
  infile.close();
  return 0; // no error!
}

void
vbmakeglm_help()
{
  printf("\nVoxBo vbmakeglm (v%s)\n",vbversion.c_str());
  printf("usage: vbmakeglm [flags] file [file ...]\n");
  printf("flags:\n");
  printf("    -i            ignore warnings, queue it anyway\n");
  printf("    -v            don't queue, just validate\n");
  printf("    -s            setup only (create filters)\n");
  printf("    -tr <rate>    set data TR (ms)\n");
  printf("    -kr <rate>    set HRF kernel sampling rate (ms)\n");
  printf("    -h            i'm lazy, show me a sample .glm file\n");
  printf("    -n <name>     only actually submit glms matching name\n");
  printf("    -x <filename> i'm really lazy, make me a sample .glm file\n");
  printf("    --run=<n>     don't queue, run now on n cores\n");
  printf("notes:\n");
  printf("  If --run is specified without =n, the default is the total number of\n");
  printf("  available cores minus 1 (or 1, if that would be 0).\n");
  printf("\n");
}

void
vbmakeglm_make_sample(const string &fname)
{
  printf("Creating sample glm file in %s...",fname.c_str());  fflush(stdout);
  FILE *fp=fopen(fname.c_str(),"a");
  if (fp) {
    vbmakeglm_sample(fp);
    fclose(fp);
    printf("done.\n");
  }
  else {
    printf("failed.\n");
  }
}

void
vbmakeglm_show_sample()
{
  printf("Here's what a valid .glm file looks like, more or less:\n\n");

  vbmakeglm_sample(stdout);
}

void
vbmakeglm_sample(FILE *fp)
{
  fprintf(fp,"###############################################################\n");
  fprintf(fp,"# Sample .glm file for VoxBo vbmakeglm.\n");
  fprintf(fp,"# At the top, put stuff that's common to all GLMS in this file.\n");
  fprintf(fp,"###############################################################\n");
  fprintf(fp,"\n");

  fprintf(fp,"# how many low frequencies do you want to filter out?\n");
  fprintf(fp,"lows 1\n\n");
  fprintf(fp,"# how many high frequencies do you want to filter out?\n");
  fprintf(fp,"highs 1\n\n");
  fprintf(fp,"# which random frequencies fo you want to filter out? (separate by spaces)\n");
  fprintf(fp,"# middles 15 16 21\n\n");
  fprintf(fp,"# how many total data points per voxel (typically time points)?\n");
  fprintf(fp,"orderg 160\n\n");
  fprintf(fp,"# how many pieces do you want to break the matrix operations into?\n");
  fprintf(fp,"pieces 10\n\n");
  fprintf(fp,"# specify a kernel for exogenous smoothing by convolution\n");
  fprintf(fp,"kernel /usr/local/VoxBo/elements/filters/Eigen1.ref\n\n");
  fprintf(fp,"# specify a model of intrinsic noise\n");
  fprintf(fp,"noisemodel /usr/local/VoxBo/elements/noisemodels/smooth_params.ref\n\n");
  fprintf(fp,"# specify the location of your G matrix, or just \"makerandfxg\" for makerandfxg\n");
  fprintf(fp,"gmatrix /data/Models/mytask.G\n");
  fprintf(fp,"# makerandfxg\n\n");
  fprintf(fp,"# the location of a reference function to copy into the GLM directory\n");
  fprintf(fp,"refname /data/Models/motor.ref\n\n");
  fprintf(fp,"# priority (1 for overnight, 2 for low, 3 for normal, 4 and 5 for various emergencies)\n");
  fprintf(fp,"pri 3\n\n");
  fprintf(fp,"# do some summary statistics on your GLM?\n");
  fprintf(fp,"audit yes\n\n");
  fprintf(fp,"# mean normalize?  usually say yes only if you have multiple runs of BOLD data\n");
  fprintf(fp,"meannorm yes\n\n");
  fprintf(fp,"# correct linear drift?  \n");
  fprintf(fp,"driftcorrect yes\n\n");
  fprintf(fp,"# your email address here\n");
  fprintf(fp,"email nobody@nowhere.com\n\n");
  fprintf(fp,"\n");

  fprintf(fp,"###############################################################\n");
  fprintf(fp,"# Then create paragraphs of specific stuff for each GLM.\n");
  fprintf(fp,"# (you can also override globals here)\n");
  fprintf(fp,"###############################################################\n");
  fprintf(fp,"\n");

  fprintf(fp,"glm larry-glm1\n");
  fprintf(fp,"dirname /data/study/larry/glm1\n");
  fprintf(fp,"scan /data/study/larry/larry01/larry01.tes\n");
  fprintf(fp,"scan /data/study/larry/larry02/larry02.tes\n");
  fprintf(fp,"end\n");
  fprintf(fp,"\n");

  fprintf(fp,"glm moe-glm1\n");
  fprintf(fp,"dirname /data/study/moe/glm1\n");
  fprintf(fp,"scan /data/study/moe/moe01/moe01.tes\n");
  fprintf(fp,"scan /data/study/moe/moe02/moe02.tes\n");
  fprintf(fp,"end\n");
  fprintf(fp,"\n");

  fprintf(fp,"glm shemp-glm1\n");
  fprintf(fp,"dirname /data/study/shemp/glm1\n");
  fprintf(fp,"scan /data/study/shemp/shemp01/shemp01.tes\n");
  fprintf(fp,"scan /data/study/shemp/shemp02/shemp02.tes\n");
  fprintf(fp,"end\n");
  fprintf(fp,"\n");
}
