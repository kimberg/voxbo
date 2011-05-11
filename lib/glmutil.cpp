
// glmutil.cpp
// Functions for glm design and manipulation
// Copyright (c) 2003-2009 by the VoxBo Development Team
//
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
// code contributed by Dongbo Hu, Dan Kimberg, and Tom King

using namespace std;

#include "glmutil.h"
#include <fstream>

// GLMParams

void
GLMParams::init()
{
  name="";
  dirname="";
  stem="";
  scanlist.clear();
  lows=highs=0;
  pieces=0;
  kernelname="";
  kerneltr=0.0;
  noisemodel="";
  refname="";
  gmatrix="";
  email="";
  pri=3;
  auditflag=1;
  meannorm=0;
  driftcorrect=0;
  emailflag=1;
  TR=0.0;
  orderg=0;
  valid=0;
  seq.init();
  rfxgflag=0;
}

void
GLMParams::Validate(bool f_ignorewarnings)
{
  int errs,warns;
  int timecount;
  Tes *mytes;
  stringstream tmps;

  timecount=0;
  errs=0;
  warns=0;
  valid=0;

  printf("[I] vbmakeglm: validating GLM %s...\n",name.c_str());

  if (!(dirname.size())) {
    printf("[E] vbmakeglm: no dirname specified for glm %s\n",name.c_str());
    errs++;
  }

  for (size_t i=0; i<scanlist.size(); i++) {
    mytes=new Tes;
    if (mytes->ReadHeader(scanlist[i].c_str())) {
      printf("[E] vbmakeglm: couldn't read 4D data from file %s.\n",scanlist[i].c_str());
      errs++;
    }
    if (!(mytes->header_valid)) {
      printf("[E] vbmakeglm: couldn't get info on data file %s.\n",scanlist[i].c_str());
      errs++;
    }
    if (mytes->header_valid) {
      timecount += mytes->dimt;
      if (i==0 && TR<1.0) {
        if (mytes->voxsize[3]>0.0)
          TR=mytes->voxsize[3];
      }
    }
    delete mytes;
  }
  // the number of time points ought to be the same as orderg
  if (orderg == 0)
    orderg=timecount;

  if (lows < 0 || (uint32)lows > (orderg/2)) {
    printf("[W] vbmakeglm: removing %d low frequencies is a little suspect\n",lows);
    warns++;
  }

  if (highs < 0 || highs > 20) {
    printf("[W] vbmakeglm: removing %d high frequencies is a little suspect\n",highs);
    warns++;
  }

  if (noisemodel.size()) {
    VB_Vector p(noisemodel.c_str());
    if (p.size() != 3) {
      printf("[E] vbmakeglm: your 1/f parameter file %s has the wrong number of elements\n",xfilename(noisemodel).c_str());
      errs++;
    }
  }

  if (kernelname.size()) {
    VB_Vector IRF;
    if (IRF.ReadFile(kernelname.c_str())) {
      tmps.str("");
      tmps << "vbmakeglm: your HRF kernel file " << kernelname << " doesn't exist.";
      printErrorMsg(VB_ERROR,tmps.str());
      errs++;
    }
    else if (IRF.size() < 6 || IRF.size() > 20) {
      tmps.str("");
      tmps << "vbmakeglm: your HRF kernel file " << kernelname << " has "
           << IRF.size() << " elements, which seems a little suspect.";
      printErrorMsg(VB_WARNING,tmps.str());
      warns++;
    }
    else if (kerneltr < 1.0) {
      string trline=GetHeader(IRF.header,"TR(msecs)");
      if (trline.size()) {
        tokenlist tmpl;
        tmpl.ParseLine(trline);
        if (tmpl.size()>1)
          kerneltr=strtod(tmpl[1]);
      }
    }
  }

  // load the G matrix, check its order and see if it has an intercept of interest
  if (gmatrix.size()) {
    VBMatrix gmat(gmatrix);
    // gmat.MakeInCore();
    if (gmat.m <=0 || gmat.n <= 0) {
      tmps.str("");
      tmps << "vbmakeglm: couldn't read G (design) matrix " << gmatrix << ".";
      printErrorMsg(VB_ERROR,tmps.str());
      errs++;
    }
    orderg=gmat.m;
    
    if (gmat.n==1 && meannorm) {
      tmps.str("");
      tmps << "vbmakeglm: you have a single covariate and the mean norm flag set - make sure you don't "
           << "mean normalize data for second tier (group rfx) analyses.";
      printErrorMsg(VB_WARNING,tmps.str());
      warns++;
    }
  }
  
  if (rfxgflag && meannorm) {
    tmps.str("");
    tmps << "vbmakeglm: you have the makerrandfxg flag and the mean norm flag set - make sure you don't "
         << "mean normalize data for second tier (group rfx) analyses.";
    printErrorMsg(VB_WARNING,tmps.str());
    warns++;
  }

  if (meannorm && noisemodel.size()==0) {
    tmps.str("");
    tmps << "vbmakeglm: you have the mean norm flag set, and no noise model - make sure you really want"
         << "to mean normalize these data.";
    printErrorMsg(VB_WARNING,tmps.str());
    warns++;
  }

  if (!meannormset) {
    tmps.str("");
    tmps << "vbmakeglm: no meannorm flag set -- defaulting to no mean normalization";
    printErrorMsg(VB_WARNING,tmps.str());
  }

  if ((int)orderg != timecount) {
    printf("[E] vbmakeglm: orderg (%d) doesn't match the number of timepoints (%d)\n",
           orderg,timecount);
    errs++;
  }

  // FIXME - should actually check order of G matrix
  if (gmatrix.size()==0 && !rfxgflag) {
    printf("[E] vbmakeglm: no valid G (design) matrix specified\n");
    errs++;
  }

  if (!errs)
    valid=1;
  if (warns && !f_ignorewarnings)
    valid=0;

  if (valid) {
    tmps.str("");
    tmps << "vbmakeglm: GLM " << name << " is good to go.";
    printErrorMsg(VB_INFO,tmps.str());
  }
  if (TR<1.0) {
    TR=2000;
    tmps.str("");
    tmps << "vbmakeglm: TR not set, using default of 2000ms.";
    printErrorMsg(VB_WARNING,tmps.str());
  }
  if (kerneltr<1.0 && kernelname.size()) {
    kerneltr=2000;
    tmps.str("");
    tmps << "vbmakeglm: HRF TR (sampling rate) not set, using default of 2000ms.";
    printErrorMsg(VB_WARNING,tmps.str());
  }
  // the following added to double-check on the TR bugs
  printf("[I] vbmakeglm: your data TR is %g\n",TR);
  printf("[I] vbmakeglm: your HRF kernel TR (sampling rate) is %g\n",kerneltr);
}

int
GLMParams::CreateGLMDir()
{
  string fname;   // temporary variable for wherever we need to build a filename
  uint32 i;
  stringstream tmps;

  stem=dirname+"/"+xfilename(dirname);
  // create directory
  createfullpath(dirname);
  // remove then create logs dir
  rmdir_force(dirname+"/logs");
  createfullpath(dirname+"/logs");
  if (!(vb_direxists(dirname)))
    return 102;
  // create sub file
  fname=stem+".sub";
  ofstream subfile((stem+".sub").c_str());
  if (!subfile)
    return 103;
  subfile << ";VB98\n;TXT1\n;\n; Subject list generated by vbmakeglm\n;\n\n";
  for (i=0; i<scanlist.size(); i++)
    subfile << scanlist[i] << endl;
  subfile.close();
  
  // create ref
  if (refname.size())
    copyfile(refname,stem+".ref");

  // create glm file for potential future use
  WriteGLMFile(stem+".glm");

  // copy G if necessary
  if (gmatrix.size() > 0) {
    // copyfile() does nothing if the two files exist with same inode
    if (copyfile(gmatrix,stem+".G"))
      return 105;
    copyfile(xsetextension(gmatrix,"preG"),stem+".preG");
  }
  else if (rfxgflag) {
    gmatrix=stem+".G";
    ofstream gstr(gmatrix.c_str(),ios::binary);
    if (gstr) {
      gstr << "VB98\nMAT1\n";
      gstr << "DataType:\tFloat\n";
      gstr << "VoxDims(XY):\t1\t" << orderg << endl << endl;
      gstr << "# This G matrix generated automatically by vbmakeglm\n\n";
      gstr << "Parameter:\t0\tInterest\tEffect\n";
      gstr << "\x0c\n";
      float pts[orderg];
      for (i=0; i<orderg; i++)
        pts[i]=1.0;
      if (my_endian() != ENDIAN_BIG)
        swap(pts,orderg);
      for (i=0; i<(orderg * sizeof(float)); i++) {
        gstr << *((unsigned char *)pts+i);
      }
      gstr.close();
    }
  }
  createsamplefiles();   // done after G matrix, so we know nvars of interest
  return 0;   // no error!
}

int
GLMParams::createsamplefiles()
{
  // first find variables of interest
  GLMInfo glmi;
  glmi.stemname=stem;
  glmi.getcovariatenames();
  string fname=dirname+"/contrasts.txt";
  vector<string> interestnames;
  if (access(fname.c_str(),R_OK) || contrasts.size()) {
    ofstream ofile(fname.c_str());
    if (ofile) {
      ofile << "# contrasts.txt\n";
      ofile << "# in this file you can define contrasts among your covariates of interest\n";
      if (glmi.cnames.size()) {
        ofile << "# your covariates of interest are:\n";
        for (size_t i=0; i<glmi.cnames.size(); i++) {
          if (glmi.cnames[i][0]=='I') {
            ofile << "#   "<<strnum(i)<<": "<<glmi.cnames[i].c_str()+1<<endl;
            interestnames.push_back(glmi.cnames[i].substr(1));
          }
        }
      }
      ofile << "# you can specify a complete contrast as follows:\n#\n";
      ofile << "# <name> <scale> vec";  // 0 0 1 0\n";
      ofile << " 1";
      for (size_t i=1; i<interestnames.size(); i++)
        ofile << " 0";
      ofile << endl << "#\n";
      ofile << "# (with one value for each covariate of interest)\n";
      ofile << "#\n";
      ofile << "# lines beginning with a '#' are comments!\n";
      ofile << "#\n";
      ofile << "# the following simple contrasts are provided for your convenience:\n";
      ofile << endl;
      for (size_t i=0; i<interestnames.size(); i++) {
        ofile << interestnames[i] << " t vec";
        for (size_t j=0; j<interestnames.size(); j++) {
          if (j==i) ofile << " 1";
          else ofile << " 0";
        }
        ofile << endl;
      }
      if (contrasts.size()) {
        ofile << "\n# the following contrasts were specified:\n";
        ofile << endl;
        for (size_t i=0; i<contrasts.size(); i++) {
          if (glmi.parsecontrast(contrasts[i]))
            printf("[W] vbgmakeglm: couldn't parse contrast: %s\n",contrasts[i].c_str());
          else
            ofile << contrasts[i] << endl;
        }
      }

      ofile.close();
    }
  }
  fname=dirname+"/averages.txt";
  if (access(fname.c_str(),R_OK)) {
    ofstream ofile(fname.c_str());
    if (ofile) {
      ofile << "# averages.txt\n";
      ofile << "# \n";
      ofile << "# In this file you can specify one or more ways to trial-average your data.\n";
      ofile << "# You can also block-average or whatever else you need, we just call it\n";
      ofile << "# trial averaging generically.\n";
      ofile << "# \n";
      ofile << "# Each trial average needs a separate section that looks like the following:\n";
      ofile << "# \n";
      ofile << "# average <name>\n";
      ofile << "#   units <time/vols>\n";
      ofile << "#   interval <ms/vols>\n";
      ofile << "#   nsamples <n>\n";
      ofile << "#   tr <ms>\n";
      ofile << "#   trial <n>...\n";
      ofile << "#   trialset <first> <interval> <count>\n";
      ofile << "# end\n";
      ofile << "# \n";
      ofile << "# Here's what these parameters mean:\n";
      ofile << "# \n";
      ofile << "# units: whether the other parameters are in volumes or seconds\n";
      ofile << "# interval: interval in time or volumes between samples within the trial\n";
      ofile << "# nsamples: number of time points to include per trial\n";
      ofile << "# tr: sets the TR if you're using time units\n";
      ofile << "#\n";
      ofile << "# The remaining options are two ways to indicate when the trials begin.\n";
      ofile << "# If your trials are evenly spaced, use 'trialset,' otherwise use 'trial'\n";
      ofile << "#\n";
      ofile << "# trialset: specify the start of the first trial, the interval between trial\n";
      ofile << "#     onsets, and the trial count\n";
      ofile << "# trial: each trial line lists one or more start times/vols for a trial\n";
      ofile << "#     (you can include multiple trial lines to help you keep the file neat)\n";
      ofile << "#\n";
      ofile << "# --> for trial and trialsets, the first volume is volume 0 (also time 0)\n";
      ofile << "# --> both time and volume values can be floating point\n";
      ofile << "#\n";
      ofile << "# Total data points for this GLM: "<<orderg<<endl;
      ofile << "# Your TR in ms: "<<TR<<endl;
      ofile << "# \n";

      ofile.close();
    }
  }
  return 0;
}

// FIXME the below is probably going to be removed

void
GLMParams::CreateGLMJobs()
{
  return;
  VBJobSpec js;
  uint32 i,rowstart,rowfinish,jobnum=0;
  bool mergeflag;
  char tmp[STRINGLEN];

  seq.name=name;

  // set pieces heuristically if the user didn't
  if (pieces==0) {
    // FIXME is this any good?
    int cellcount=600000;
    pieces=(int)ceil((double)orderg*orderg/cellcount);
    if (pieces > orderg) pieces=orderg;
    if (pieces<1) pieces=1;
  }
  uint32 rowsperjob=orderg/pieces;
  // are we going to need the merge jobs?
  if (rowsperjob < orderg)
    mergeflag=1;
  else
    mergeflag=0;

  // make the exofilt
  js.init();
  js.jobtype="vb_makefilter";
  js.arguments["outfile"]=stem+".ExoFilt";
  // sprintf(tmp,"lowflag -lf %d",lows);
  js.arguments["lowflag"]=(string)"-lf "+strnum(lows);
  if (middles.size())
    js.arguments["middleflag"]=(string)"-mf "+middles;
  else
    js.arguments["middleflag"]="";
  sprintf(tmp,"highflag -hf %d",highs);
  js.arguments["highflag"]=(string)"-hf "+strnum(highs);
  if (kernelname.size())
    js.arguments["kernelflag"]=(string)"-k "+kernelname;
  else
    js.arguments["kernelflag"]="";
  js.arguments["ndata"]=strnum(orderg);
  sprintf(tmp,"tr %f",TR);
  js.arguments["tr"]=strnum(TR);
  js.magnitude=0;    // set it to something!
  js.name="make exofilt";
  js.jnum=jobnum++;
  int n_exofilt=js.jnum;
  seq.addJob(js);

  // make the noisemodel
  js.init();
  js.jobtype="vb_makenoisemodel";
  js.arguments["outfile"]=stem+".IntrinCor";
  if (noisemodel.size())
    js.arguments["noisemodelflag"]="-n "+noisemodel;
  js.arguments["ndata"]=strnum(orderg);
  js.arguments["tr"]=strnum(TR);
  js.magnitude=0;    // set it to something!
  js.name="make noisemodel";
  js.jnum=jobnum++;
  int n_noisemodel=js.jnum;
  seq.addJob(js);

  // make the KG matrix
  js.init();
  js.jobtype="makematkg";
  js.arguments["stem"]=stem;
  js.magnitude=0;    // set it to something!
  js.name="GLM-KG";
  js.waitfor.insert(n_exofilt);
  js.jnum=jobnum++;
  int n_kg=js.jnum;
  seq.addJob(js);

  // make the F1 matrix
  js.init();
  js.jobtype="matpinv";
  js.arguments["in"]=stem+".KG";
  js.arguments["out"]=stem+".F1";
  js.magnitude=0;    // set it to something!
  js.name="GLM-F1";
  js.waitfor.insert(n_kg);
  js.jnum=jobnum++;
  int n_f1=js.jnum;
  seq.addJob(js);

  // make the R matrix = I-(KG)(F1)
  js.init();
  js.jobtype="matimxy";
  js.arguments["in1"]=stem+".KG";
  js.arguments["in2"]=stem+".F1";
  js.arguments["out"]=stem+".R";
  js.magnitude=0;    // set it to something!
  js.name="GLM-R";
  js.waitfor.insert(n_f1);
  js.jnum=jobnum++;
  int n_r=js.jnum;
  seq.addJob(js);

  // create a K matrix
  js.init();
  js.jobtype="makematk";
  js.arguments["stem"]=stem;
  js.magnitude=0;    // set it to something!
  js.name="GLM-K";
  js.waitfor.insert(n_exofilt);
  js.waitfor.insert(n_noisemodel);
  js.jnum=jobnum++;
  int n_k=js.jnum;
  seq.addJob(js);

  // create an empty V matrix
  js.init();
  js.jobtype="matzeros";
  js.arguments["name"]=stem+".V"; 
  js.arguments["cols"]=strnum(orderg);
  js.arguments["rows"]=strnum(orderg);
  js.magnitude=0;    // set it to something!
  js.name="GLM-Vcreate";
  js.jnum=jobnum++;
  int n_vcreate=js.jnum;
  seq.addJob(js);

  // build V matrix (V=KKt)
  set<int32> n_vpieces;
  rowstart=0;
  i=0;
  int n_v;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="matxxt";
    js.arguments["in"]=stem+".K";
    js.arguments["out"]=stem+".V";
    js.arguments["col1"]=strnum(rowstart);
    js.arguments["col2"]=strnum(rowfinish);
    js.magnitude=0;    // set it to something!
    sprintf(tmp,"GLM-V%d",i++);
    js.name=tmp;
    js.waitfor.insert(n_k);
    js.waitfor.insert(n_vcreate);
    js.jnum=jobnum;
    n_vpieces.insert(js.jnum);
    n_v=js.jnum;  // in case it's just one piece
    seq.addJob(js);
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge V
  if (mergeflag) {
    js.init();
    js.jobtype="matmerge";
    js.arguments["out"]=stem+".V";
    js.name="GLM-MergeV";
    js.waitfor=n_vpieces;
    js.jnum=jobnum++;
    n_v=js.jnum;
    seq.addJob(js);
  }
  
  // create F3
  js.init();
  js.jobtype="matf3";
  js.arguments["vmatrix"]=stem+".V";
  js.arguments["kgmatrix"]=stem+".KG";
  js.arguments["outfile"]=stem+".F3";
  js.magnitude=0;    // set it to something!
  js.name="GLM-makeF3";
  js.waitfor.insert(n_kg);
  js.waitfor.insert(n_v);
  js.jnum=jobnum++;
  int n_f3=js.jnum;
  seq.addJob(js);
  
  // create an empty RV matrix
  js.init();
  js.jobtype="matzeros";
  js.arguments["name"]=stem+".RV";
  js.arguments["cols"]=strnum(orderg);
  js.arguments["rows"]=strnum(orderg);
  js.magnitude=0;    // set it to something!
  js.name="GLM-RVcreate";
  js.jnum=jobnum++;
  int n_rvcreate=js.jnum;
  seq.addJob(js);

  // build RV matrix
  set<int32> n_rvpieces;
  rowstart=0;
  int n_rv;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="matxy";
    js.arguments["in1"]=stem+".R";
    js.arguments["in2"]=stem+".V";
    js.arguments["out"]=stem+".RV";
    js.arguments["col1"]=strnum(rowstart);
    js.arguments["col2"]=strnum(rowfinish);
    js.magnitude=0;    // set it to something!
    sprintf(tmp,"GLM-RV%d",i);
    js.name=tmp;
    js.waitfor.insert(n_r);
    js.waitfor.insert(n_v);
    js.waitfor.insert(n_rvcreate);
    js.jnum=jobnum;
    n_rv=js.jnum;
    seq.addJob(js);
    n_rvpieces.insert(js.jnum);
    n_rv=js.jnum;    // in case it's one piece
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RV
  if (mergeflag) {
    js.init();
    js.jobtype="matmerge";
    js.arguments["out"]=stem+".RV";
    js.name="GLM-MergeRV";
    js.waitfor=n_rvpieces;
    js.jnum=jobnum++;
    n_rv=js.jnum;
    seq.addJob(js);
  }
  
  // create an empty RVRV matrix
  js.init();
  js.jobtype="matzeros";
  js.arguments["name"]=stem+".RVRV";
  js.arguments["cols"]=strnum(orderg);
  js.arguments["rows"]=strnum(orderg);
  js.magnitude=0;    // set it to something!
  js.name="GLM-RVRVcreate";
  js.jnum=jobnum++;
  int n_rvrvcreate=js.jnum;
  seq.addJob(js);

  // build RVRV
  set<int32> n_rvrvpieces;
  int n_rvrv;
  rowstart=0;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="matxx";
    js.arguments["in"]=stem+".RV";
    js.arguments["out"]=stem+".RVRV";
    js.arguments["col1"]=strnum(rowstart);
    js.arguments["col2"]=strnum(rowfinish);
    js.magnitude=0;    // set it to something!
    sprintf(tmp,"GLM-RVRV%d",i);
    js.name=tmp;
    js.waitfor.insert(n_rvrvcreate);
    js.waitfor.insert(n_rv);
    js.jnum=jobnum;
    n_rvrvpieces.insert(js.jnum);
    n_rvrv=js.jnum;
    seq.addJob(js);
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RVRV
  if (mergeflag) {
    js.init();
    js.jobtype="matmerge";
    js.arguments["out"]=stem+".RVRV";
    js.name="GLM-MergeRVRV";
    js.waitfor=n_rvrvpieces;
    js.jnum=jobnum++;
    n_rvrv=js.jnum;
    seq.addJob(js);
  }

  // traces
  js.init();
  js.jobtype="comptraces";
  js.arguments["stem"]=stem;
  js.magnitude=0;    // set it to something!
  js.name="GLM-traces";
  js.waitfor.insert(n_rvrv);
  js.jnum=jobnum++;
  int n_traces=js.jnum;
  seq.addJob(js);
  
  // tes regression steps
  set<int32> n_regstep;
  for (i=0; i<pieces; i++) {
    js.init();
    js.jobtype="vbregress";
    js.arguments["stem"]=stem;
    js.arguments["steps"]=strnum(pieces);
    js.arguments["index"]=strnum(i+1);
    string flags;
    if (this->meannorm)
      flags+="-m ";
    if (this->driftcorrect)
      flags+="-d";
    js.arguments["flags"]=flags;
    js.magnitude=0;    // set it to something!
    sprintf(tmp,"regress(%d/%d)",i+1,pieces);
    js.name=tmp;
    js.waitfor.insert(n_traces);
    js.waitfor.insert(n_f1);
    js.waitfor.insert(n_r);
    js.waitfor.insert(n_exofilt);
    js.jnum=jobnum;
    n_regstep.insert(js.jnum);
    seq.addJob(js);
    jobnum++;
  } // for i
  
  if (pieces>1) {
    js.init();
    js.jobtype="vbmerge4d";
    js.arguments["stem"]=stem;
    js.arguments["ext"]="prm";
    js.magnitude=0;    // set it to something!
    js.name="mergeparams";
    js.waitfor=n_regstep;
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    js.init();
    js.jobtype="vbmerge4d";
    js.arguments["stem"]=stem;
    js.arguments["ext"]="res";
    js.magnitude=0;    // set it to something!
    js.name="mergeparams";
    js.waitfor=n_regstep;
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    // make these two jobs stand in for regstep in the waitfor chain
    n_regstep.clear();
    n_regstep.insert(jobnum-2);
    n_regstep.insert(jobnum-1);
  }
  
  // create smoothness estimate
  js.init();
  js.jobtype="vbse";
  js.arguments["stem"]=stem;
  js.magnitude=0;    // set it to something!
  js.name="vbse";
  js.waitfor=n_regstep;
  js.jnum=jobnum;
  int n_se=js.jnum;
  seq.addJob(js);
  jobnum++;

  // audit?
  int n_audit;
  if (auditflag) {
    js.init();
    js.jobtype="vb_auditglm";
    js.arguments["glmdir"]=xdirname(stem);
    js.magnitude=0;    // set it to something!
    js.name="GLM-audit";
    js.waitfor=n_regstep;
    js.waitfor.insert(n_se);
    js.jnum=jobnum++;
    n_audit=js.jnum;
    seq.addJob(js);
  }
  
  // notify?
  if (emailflag && email.size()) {
    js.init();
    js.jobtype="notify";
    js.arguments["email"]=email;
    js.arguments["msg"]="Your GLM has been solved.";
    js.magnitude=0;    // set it to something!
    js.name="Notify";
    js.waitfor.insert(n_se);
    if (auditflag)
      js.waitfor.insert(n_audit);
    js.waitfor.insert(n_f3);
    js.jnum=jobnum++;
    seq.addJob(js);
  }
}

void
GLMParams::CreateGLMJobs2()
{
  VBJobSpec js;
  uint32 i,rowstart,rowfinish,jobnum=0;
  bool mergeflag;
  char tmp[STRINGLEN];

  seq.name=name;

  // set pieces heuristically if the user didn't
  if (pieces==0) {
    // FIXME is this any good?
    int cellcount=600000;
    pieces=(int)ceil((double)orderg*orderg/cellcount);
  }
  if (pieces > orderg) pieces=orderg;
  if (pieces<1) pieces=1;
  uint32 rowsperjob=orderg/pieces;
  // are we going to need the merge jobs?
  if (rowsperjob < orderg)
    mergeflag=1;
  else
    mergeflag=0;

  // make the exofilt
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmakefilter -e %s.ExoFilt -lf %d -hf %d %s %s -t %d %f")
                              %stem%lows%highs%(middles.size()?"-mf "+middles:"")
                              %(kernelname.size()?"-k "+kernelname:"")
                              %orderg%TR);
  js.name="make exofilt";
  js.jnum=jobnum++;
  int n_exofilt=js.jnum;
  seq.addJob(js);

  // make the noisemodel
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmakefilter -i %s.IntrinCor %s -t %d %f")
                              %stem%(noisemodel.size()?"-n "+noisemodel:"")%orderg%TR);
  js.name="make noisemodel";
  js.jnum=jobnum++;
  int n_noisemodel=js.jnum;
  seq.addJob(js);

  // make the KG matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("makematkg -m %s")%stem);
  js.name="GLM-KG";
  js.waitfor.insert(n_exofilt);
  js.jnum=jobnum++;
  int n_kg=js.jnum;
  seq.addJob(js);

  // make the F1 matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -pinv %1%.KG %1%.F1")%stem);
  js.name="GLM-F1";
  js.waitfor.insert(n_kg);
  js.jnum=jobnum++;
  int n_f1=js.jnum;
  seq.addJob(js);

  // make the R matrix = I-(KG)(F1)
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -imxy %1%.KG %1%.F1 %1%.R")%stem);
  js.name="GLM-R";
  js.waitfor.insert(n_f1);
  js.jnum=jobnum++;
  int n_r=js.jnum;
  seq.addJob(js);

  // create a K matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("makematk -m %s")%stem);
  js.name="GLM-K";
  js.waitfor.insert(n_exofilt);
  js.waitfor.insert(n_noisemodel);
  js.jnum=jobnum++;
  int n_k=js.jnum;
  seq.addJob(js);

  // create an empty V matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -zeros %s.V %d %d")%stem%orderg%orderg);
  js.name="GLM-Vcreate";
  js.jnum=jobnum++;
  int n_vcreate=js.jnum;
  seq.addJob(js);

  // build V matrix (V=KKt)
  set<int32> n_vpieces;
  rowstart=0;
  i=0;
  int n_v;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -xyt %1%.K %1%.K %1%.V %2% %3%")%stem%rowstart%rowfinish);
    sprintf(tmp,"GLM-V%d",i++);
    js.name=tmp;
    js.waitfor.insert(n_k);
    js.waitfor.insert(n_vcreate);
    js.jnum=jobnum;
    n_vpieces.insert(js.jnum);
    n_v=js.jnum;  // in case it's just one piece
    seq.addJob(js);
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge V
  if (mergeflag) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -assemblecols %s.V")%stem);
    js.name="GLM-MergeV";
    js.waitfor=n_vpieces;
    js.jnum=jobnum++;
    n_v=js.jnum;
    seq.addJob(js);
  }
  
  // create F3
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -f3 %1%.V %1%.KG %1%.F3")%stem);
  js.name="GLM-makeF3";
  js.waitfor.insert(n_kg);
  js.waitfor.insert(n_v);
  js.jnum=jobnum++;
  int n_f3=js.jnum;
  seq.addJob(js);
  
  // create an empty RV matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -zeros %s.RV %d %d")%stem%orderg%orderg);
  js.name="GLM-RVcreate";
  js.jnum=jobnum++;
  int n_rvcreate=js.jnum;
  seq.addJob(js);

  // build RV matrix
  set<int32> n_rvpieces;
  rowstart=0;
  int n_rv;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -xyt %1%.R %1%.V %1%.RV %2% %3%")%stem%rowstart%rowfinish);
    sprintf(tmp,"GLM-RV%d",i);
    js.name=tmp;
    js.waitfor.insert(n_r);
    js.waitfor.insert(n_v);
    js.waitfor.insert(n_rvcreate);
    js.jnum=jobnum;
    n_rv=js.jnum;
    seq.addJob(js);
    n_rvpieces.insert(js.jnum);
    n_rv=js.jnum;    // in case it's one piece
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RV
  if (mergeflag) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -assemblecols %s.RV")%stem);
    js.name="GLM-MergeRV";
    js.waitfor=n_rvpieces;
    js.jnum=jobnum++;
    n_rv=js.jnum;
    seq.addJob(js);
  }
  
  // create an empty RVRV matrix
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbmm2 -zeros %s.RVRV %d %d")%stem%orderg%orderg);
  js.name="GLM-RVRVcreate";
  js.jnum=jobnum++;
  int n_rvrvcreate=js.jnum;
  seq.addJob(js);

  // build RVRV
  set<int32> n_rvrvpieces;
  int n_rvrv;
  rowstart=0;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -xy %1%.RV %1%.RV %1%.RVRV %2% %3%")%stem%rowstart%rowfinish);
    sprintf(tmp,"GLM-RVRV%d",i);
    js.name=tmp;
    js.waitfor.insert(n_rvrvcreate);
    js.waitfor.insert(n_rv);
    js.jnum=jobnum;
    n_rvrvpieces.insert(js.jnum);
    n_rvrv=js.jnum;
    seq.addJob(js);
    jobnum++;
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RVRV
  if (mergeflag) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmm2 -assemblecols %s.RVRV")%stem);
    js.name="GLM-MergeRVRV";
    js.waitfor=n_rvrvpieces;
    js.jnum=jobnum++;
    n_rvrv=js.jnum;
    seq.addJob(js);
  }

  // traces
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("comptraces -m %s")%stem);
  js.name="GLM-traces";
  js.waitfor.insert(n_rvrv);
  js.jnum=jobnum++;
  int n_traces=js.jnum;
  seq.addJob(js);
  
  // tes regression steps
  set<int32> n_regstep;
  string flags;
  if (this->meannorm)
    flags+="-m ";
  if (this->driftcorrect)
    flags+="-d";
  for (i=0; i<pieces; i++) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbregress %s -p %d %d %s")
                                %stem%(i+1)%pieces%flags);
    sprintf(tmp,"regress(%d/%d)",i+1,pieces);
    js.name=tmp;
    js.waitfor.insert(n_traces);
    js.waitfor.insert(n_f1);
    js.waitfor.insert(n_r);
    js.waitfor.insert(n_exofilt);
    js.jnum=jobnum;
    n_regstep.insert(js.jnum);
    seq.addJob(js);
    jobnum++;
  } // for i
  
  if (pieces>1) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmerge4d %1%.prm_part_* -o %1%.prm")%stem);
    js.name="mergeparams";
    js.waitfor=n_regstep;
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("rm -v %s.prm_part_*")%stem);
    js.name="mergeparams";
    js.waitfor.insert(jobnum-1);
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("vbmerge4d %1%.res_part_* -o %1%.res")%stem);
    js.name="mergeparams";
    js.waitfor=n_regstep;
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("rm -v %s.res_part_*")%stem);
    js.name="mergeparams";
    js.waitfor.insert(jobnum-1);
    js.jnum=jobnum;
    seq.addJob(js);
    jobnum++;

    // make these four jobs stand in for regstep in the waitfor chain
    n_regstep.clear();
    n_regstep.insert(jobnum-4);
    n_regstep.insert(jobnum-3);
    n_regstep.insert(jobnum-2);
    n_regstep.insert(jobnum-1);
  }
  
  // create smoothness estimate
  js.init();
  js.jobtype="shellcommand";
  js.arguments["command"]=str(format("vbse %1%.res %1%.se")%stem);
  js.name="vbse";
  js.waitfor=n_regstep;
  js.jnum=jobnum;
  int n_se=js.jnum;
  seq.addJob(js);
  jobnum++;

  // audit?
  int n_audit;
  if (auditflag) {
    js.init();
    js.jobtype="shellcommand";
    js.arguments["command"]=str(format("glminfo -r %1% > %1%/audit.txt")%xdirname(stem));
    js.name="GLM-audit";
    js.waitfor=n_regstep;
    js.waitfor.insert(n_se);
    js.jnum=jobnum++;
    n_audit=js.jnum;
    seq.addJob(js);
  }
  
  // notify?
  if (emailflag && email.size()) {
    js.init();
    js.jobtype="notify";
    js.arguments["email"]=email;
    js.arguments["msg"]="Your GLM has been solved.";
    js.magnitude=0;    // set it to something!
    js.name="Notify";
    js.waitfor.insert(n_se);
    if (auditflag)
      js.waitfor.insert(n_audit);
    js.waitfor.insert(n_f3);
    js.jnum=jobnum++;
    seq.addJob(js);
  }
  seq.priority=pri;
  seq.email=email;
  for (SMI j=seq.specmap.begin(); j!=seq.specmap.end(); j++) {
    j->second.dirname=dirname;
    j->second.logdir=dirname+"/logs";
  }
}

vector<string>
GLMParams::CreateGLMScript()
{
  VBJobSpec js;
  uint32 i,rowstart,rowfinish;
  bool mergeflag;
  string tmps;

  // set pieces heuristically if the user didn't
  if (pieces==0) {
    // FIXME is this any good?
    int cellcount=600000;
    pieces=(int)ceil((double)orderg*orderg/cellcount);
    if (pieces > orderg) pieces=orderg;
    if (pieces<1) pieces=1;
  }
  uint32 rowsperjob=orderg/pieces;
  // are we going to need the merge jobs?
  if (rowsperjob < orderg)
    mergeflag=1;
  else
    mergeflag=0;

  vector<string> commandlist;

  // make the exofilt
  tmps=(format("vbmakefilter -e %s.ExoFilt -lf %d -hf %d %s %s -t %d %f")
        %stem%lows%highs%(middles.size()?"-mf "+middles:"")
        %(kernelname.size()?"-k "+kernelname:"")
        %orderg%TR).str();
  commandlist.push_back(tmps);
  // make the noisemodel
  tmps=(format("vbmakefilter -i %s.IntrinCor %s -t %d %f")
        %stem%(noisemodel.size()?"-n "+noisemodel:"")%orderg%TR).str();
  commandlist.push_back(tmps);
  // make the KG matrix
  tmps=(format("makematkg -m %s")%stem).str();
  commandlist.push_back(tmps);
  // make the F1 matrix
  tmps=(format("vbmm2 -pinv %1%.KG %1%.F1")%stem).str();
  commandlist.push_back(tmps);
  // make the R matrix = I-(KG)(F1)
  tmps=(format("vbmm2 -imxy %1%.KG %1%.F1 %1%.R")%stem).str();
  commandlist.push_back(tmps);
  // create a K matrix
  tmps=(format("makematk -m %s")%stem).str();
  commandlist.push_back(tmps);
  // create an empty V matrix
  tmps=(format("vbmm2 -zeros %s.V %d %d")%stem%orderg%orderg).str();
  commandlist.push_back(tmps);

  // build V matrix (V=KKt)
  set<int32> n_vpieces;
  rowstart=0;
  i=0;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    js.init();
    tmps=(format("vbmm2 -xyt %1%.K %1%.K %1%.V %2% %3%")%stem%rowstart%rowfinish).str();
    commandlist.push_back(tmps);
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge V
  if (mergeflag) {
    tmps=(format("vbmm2 -assemblecols %s.V")%stem).str();
    commandlist.push_back(tmps);
  }
  
  // create F3
  tmps=(format("vbmm2 -f3 %1%.V %1%.KG %1%.F3")%stem).str();
  commandlist.push_back(tmps);
  
  // create an empty RV matrix
  tmps=(format("vbmm2 -zeros %s.RV %d %d")%stem%orderg%orderg).str();
  commandlist.push_back(tmps);

  // build RV matrix
  rowstart=0;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    tmps=(format("vbmm2 -xyt %1%.R %1%.V %1%.RV %2% %3%")%stem%rowstart%rowfinish).str();
    commandlist.push_back(tmps);
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RV
  if (mergeflag) {
    tmps=(format("vbmm2 -assemblecols %s.RV")%stem).str();
    commandlist.push_back(tmps);
  }
  
  // create an empty RVRV matrix
  tmps=(format("vbmm2 -zeros %s.RVRV %d %d")%stem%orderg%orderg).str();
  commandlist.push_back(tmps);

  // build RVRV
  rowstart=0;
  do {
    rowfinish=rowstart+rowsperjob;
    if (rowfinish > orderg-1)
      rowfinish=orderg-1;
    tmps=(format("vbmm2 -xy %1%.RV %1%.RV %1%.RVRV %2% %3%")%stem%rowstart%rowfinish).str();
    commandlist.push_back(tmps);
    rowstart+=rowsperjob+1;
  } while (rowstart < orderg);

  // merge RVRV
  if (mergeflag) {
    tmps=(format("vbmm2 -assemblecols %s.RVRV")%stem).str();
    commandlist.push_back(tmps);
  }

  // traces
  tmps=(format("comptraces -m %s")%stem).str();
  commandlist.push_back(tmps);
  
  // tes regression steps
  string flags;
  if (this->meannorm)
    flags+="-m ";
  if (this->driftcorrect)
    flags+="-d";
  for (i=0; i<pieces; i++) {
    tmps=(format("vbregress %s -p %d %d %s")
          %stem%(i+1)%pieces%flags).str();
    commandlist.push_back(tmps);
  }
  
  if (pieces>1) {
    tmps=str((format("vbmerge4d %1%.prm_part_* -o %1%.prm")%stem));
    commandlist.push_back(tmps);
    tmps=str((format("rm -v %s.prm_part_*")%stem));
    commandlist.push_back(tmps);

    tmps=str((format("vbmerge4d %1%.res_part_* -o %1%.res")%stem));
    commandlist.push_back(tmps);
    tmps=str((format("rm -v %s.res_part_*")%stem));
    commandlist.push_back(tmps);
  }
  
  // create smoothness estimate
  tmps=str((format("vbse %1%.res %1%.se")%stem));
  commandlist.push_back(tmps);

  // audit?
  if (auditflag) {
    tmps=str((format("glminfo -r %1% > %1%/audit.txt")%xdirname(stem)));
    commandlist.push_back(tmps);
  }
  return commandlist;
}

int
GLMParams::WriteGLMFile(string fname)
{
  FILE *fp;
  if (fname.empty())
    fname=stem+".glm";
  fp=fopen(fname.c_str(),"w");
  if (!fp) {
    printf("[E] vbmakeglm: couldn't create glm file %s\n",fname.c_str());
    return 103;
  }
  fprintf(fp,"lows %d\n",lows);
  fprintf(fp,"highs %d\n",highs);
  if (middles.size())
    fprintf(fp,"middles %s\n",middles.c_str());
  fprintf(fp,"orderg %d\n",orderg);
  fprintf(fp,"pieces %d\n",pieces);
  fprintf(fp,"kernel %s\n",kernelname.c_str());
  fprintf(fp,"noisemodel %s\n",noisemodel.c_str());
  if (rfxgflag)
    fprintf(fp,"makerandfxg\n");
  else
    fprintf(fp,"gmatrix %s\n",gmatrix.c_str());
  if (refname.size())
    fprintf(fp,"refname %s\n",refname.c_str());
  fprintf(fp,"pri %d\n",pri);
  fprintf(fp,"audit %s\n",(auditflag ? "yes" : "no"));
  fprintf(fp,"meannorm %s\n",(meannorm ? "yes" : "no"));
  fprintf(fp,"driftcorrect %s\n",(driftcorrect ? "yes" : "no"));
  fprintf(fp,"email %s\n",email.c_str());
  fprintf(fp,"\n");
  fprintf(fp,"glm %s\n",name.c_str());
  fprintf(fp,"dirname %s\n",dirname.c_str());
  for (size_t i=0; i<scanlist.size(); i++)
    fprintf(fp,"scan %s\n",scanlist[i].c_str());
  fprintf(fp,"end\n");
  fclose(fp);
  return 0;  
}

void
GLMParams::FixRelativePaths()
{
  string path=xgetcwd()+"/";
  dirname=xabsolutepath(dirname);
  kernelname=xabsolutepath(kernelname);
  noisemodel=xabsolutepath(noisemodel);
  refname=xabsolutepath(refname);
  gmatrix=xabsolutepath(gmatrix);
  for (size_t i=0; i<scanlist.size(); i++)
    scanlist[i]=xabsolutepath(scanlist[i]);
}

/*****************************************************************************
 * Functions to read a condition function with strings 
 * (originally from gdw's condition function loading)
 * Added by Dongbo on March 16, 2004
 *****************************************************************************/

/* Main function to translate a condition function into vb_vector */
int getCondVec(const char *condFile, tokenlist &condKey, VB_Vector *condVec)
{
  tokenlist headerKey, output;
  int readStat = readCondFile(headerKey, output, condFile);
  if (readStat == -1)
    return -1;

  unsigned condLength = output.size();
  tokenlist contentKey = getContentKey(output);
  int cmpStat = cmpElement(headerKey, contentKey);
  if (cmpStat == -1) {
    sortElement(contentKey);
    for (size_t i=0; i<contentKey.size(); i++)
      condKey.Add(contentKey(i));
  }
  else if (cmpStat == -2)
    return -2;
  else if (cmpStat == 1)
    return 1;
  else {
    for (size_t i=0; i<headerKey.size(); i++) 
      condKey.Add(headerKey(i));
  }

  // condVec is the original vector converted from the condition function
  condVec->resize(condLength);
  for (size_t i = 0; i < condLength; i++) {
    for (size_t j = 0; j < condKey.size(); j++) {
      if (strcmp(output(i), condKey[j].c_str()) == 0) {
        condVec->setElement(i, j);
        break;
      }
      else
        continue;
    }
  }

  return 0;
}

/* readCondFile() reads an input condition function file and save each uncommented 
 * line into a tokenlist object. It also checks if the header comments have any 
 * specific condition label keys defined. */
int readCondFile(tokenlist &headerKey, tokenlist &output, const char *condFile)
{
  FILE *fp;
  char line[512]; // Assume each line has 512 charactoers or less

  fp = fopen(condFile,"r");
  if (!fp)
    return -1;    // returns -1 if file not readable

  string tmpString1, tmpString11, tmpString2, tmpString22, tmpString3;
  while (fgets(line,512,fp)) {
    if (strchr(";#%\n",line[0])) {
      stripchars(line,"\n");
      tmpString1 = &line[0];

      // If a comment line is long enough, check whether it has user-defined condition labels
      if (tmpString1.length() > 11) {
	tmpString11 = tmpString1.substr(1, tmpString1.length() - 1); // Remove comment character
	tmpString2 = xstripwhitespace(tmpString11);                  // Simplify white spaces

	// If tmpString2 started with "condition:", get the string after that and save as a condition label
	tmpString22 = tmpString2.substr(0, 10);
	tmpString22=vb_tolower(tmpString22);
	
	if (tmpString22 == "condition:") {
	  tmpString3 = xstripwhitespace(tmpString2.substr(10, tmpString2.length() - 10));
	  headerKey.Add(tmpString3);
	}
      }
      continue;
    }

    stripchars(line,"\n");
    // If the line starts with space (32) or tab (9) keys, delete them
    while(int(line[0]) == 32 || int(line[0]) == 9) { 
      for (size_t n = 0; n < strlen(line); n++)
        line[n] = line[n + 1];
    }    
    output.Add(line); // Thanks to Dan's tokenlist class
  }

  fclose(fp);
  return 0;    // no error!

}

/* This function scans each element in condFunctand records unique condition keys in the 
 * order of their appearance. Keys are saved in condKeyInFile, which is another tokenlist. */
tokenlist getContentKey(tokenlist &inputLine)
{
  tokenlist condKeyInFile = tokenlist();
  string keyStr;
  int condLength = inputLine.size();
  // If one of the keys is "0" or "baseline", set it as the first condition key
  for (int i = 0; i < condLength; i++) {
    keyStr = inputLine(i);
    if (keyStr == "0" || keyStr == "baseline") {
      condKeyInFile.Add(keyStr);
      break;
    }
    else if (i == condLength - 1)
      condKeyInFile.Add(inputLine(0));
    else
      continue;
  }

  for (int i = 0; i < condLength; i++) {
    if (cmpString(inputLine(i), condKeyInFile)) 
      condKeyInFile.Add(inputLine(i));
  }

  return condKeyInFile;
}

/* This method is to compare an input string with elements in condKey, which is a tokenlist object. 
 * Returns 0 if the input string is equal to any of the elements in condKey
 * Returns 1 if the input string isn't equal to any of the elements in condKey */
int cmpString(const char *inputString, deque<string> inputList)
{
  for (unsigned i = 0; i < inputList.size(); i++) {
    if (strcmp(inputList[i].c_str(), inputString) == 0)
      return 0;
  }

  return 1;
}

/* This function sorts the elements in a tokenlist object alphabetically.
 * First it checks whether the first element is "0" or "baseline". 
 * If yes, it will keep it and sort the rest of the elements alphabetically;
 * If not, it will sort all elements alphabetically. */
void sortElement(tokenlist &inputList)
{
  const char * element0 = inputList(0);
  if (strcmp(element0, "0") == 0 || strcmp(element0, "baseline") == 0) {
    tokenlist otherElmt;
    for (size_t i = 1; i < inputList.size(); i++)
      otherElmt.Add(inputList(i));
    otherElmt.Sort(alpha_sorter);

    inputList.clear();
    inputList.Add(element0);
    for (size_t i = 0; i < otherElmt.size(); i++)
      inputList.Add(otherElmt(i));
  }
  else
    inputList.Sort(alpha_sorter);
}

/* cmpElement() compares elements in two input tokenlists. 
 * If each element in input1 can be found in input2 and vice versa 
 * (without considering the order of elements), it will return 0; 
 * Otherwise, it will:
 * returns  0 if two input tokenlists are exactly same
 * returns -1 if the first tokenlist doesn't have any elements;
 * returns -2 if two tokenlist have different number of elements
 * returns  1 if some elements are different. */
int cmpElement(deque<string> input1, deque<string> input2)
{
  if (!input1.size())
    return -1;

  if (input1.size() != input2.size())
    return -2;

  for (unsigned i = 0; i < input1.size(); i++) {
    if (cmpString(input1[i].c_str(), input2) == 0 && cmpString(input2[i].c_str(), input1) == 0)
      continue;
    else 
      return 1;
  }

  return 0;
}

/* getCondLabel() reads a text file and saves each uncommented line into a tokenlist object 
 * It is written for loading condition labels from a certain file */
int getCondLabel(tokenlist &outputToken, const char *inputFile)
{
//  outputToken = tokenlist();
  const char *condFile = inputFile;
  FILE *fp;
  char line[512]; // Assume each line has 512 charactoers or less

  fp = fopen(condFile,"r");
  if (!fp)
    return -1; // returns -1 if file not readable

  while (fgets(line,512,fp)) {
    // Disregard the line if it starts with these characters: ;#%\n (notice that space is allowed!)
    if (strchr(";#%\n",line[0])) 
      continue;
    stripchars(line,"\n");

    // If the line starts with space (32) or tab (9) keys, delete them
    while(int(line[0]) == 32 || int(line[0]) == 9) { 
	for (size_t n = 0; n < strlen(line); n++)
	  line[n] = line[n + 1];
    }    
    outputToken.Add(line); // Thanks to Dan's tokenlist class
  }

  fclose(fp);
  return 0;    // no error!
}

/* Function to check an output filename's status, 
 * originally written for vbfit interactive mode */
// FIXME doesn't belong here!  (or anywhere)
int checkOutputFile(const char *outFilename, bool ovwFlag)
{
  bool f_exists=vb_fileexists(outFilename);
  bool pw=0;
  if (!access(xdirname(outFilename).c_str(),W_OK))
    pw=1;
  if (f_exists && !pw)
    return 0;    // returns 0 if the file exists and parent directory isn't writable
  else if (f_exists && pw && !ovwFlag)
    return 1;    // returns 1 if the file exists, parent directory is writable, but overwrite isn't allowed

  else if (!f_exists && !pw)
    return 2;    // returns 2 if the file doesn't exist but parent directory isn't writable

  else if (f_exists && pw && ovwFlag) 
    return 3;    // returns 3 if the file exists, parent directory is writable and overwrite is allowed

  else
    return 4;    // returns 4 if the file doesn't exist and parent directory is writable
}

/* This function will upsample an input vector by a certain ratio 
 * and return the new vector */
VB_Vector * upSampling(VB_Vector *inputVector, int upRatio)
{
  int origSize = inputVector->getLength();
  int newSize = upRatio * origSize;
  VB_Vector *newVector = new VB_Vector(newSize);
  double tmpValue;
  int oldIndex;

  for (int newIndex = 0; newIndex < newSize; newIndex++) {
    oldIndex = newIndex / upRatio;
    tmpValue = inputVector->getElement(oldIndex);
    for (int j = 0; j < upRatio; j++) 
      newVector->setElement(newIndex, tmpValue);
  }
	
  return newVector;
}

/* This function will downsample an input vector by a certain ratio and returns the new vector */
VB_Vector * downSampling(VB_Vector *inputVector, int downRatio)
{
  int origSize =  inputVector->getLength();
  int newSize = origSize / downRatio;
  VB_Vector *newVector = new VB_Vector(newSize);
  double tmpValue;

  for (int newIndex = 0; newIndex < newSize; newIndex++) {
    tmpValue = inputVector->getElement(newIndex * downRatio);
    newVector->setElement(newIndex, tmpValue);
  }
  
  return newVector;
}

/* getConv() is a generic function to convolve an input VB_Vector with inputConv 
 * by the input sampling rate. It calls fftConv() for basic convolution calculation
 * using Geoff's fft/ifft combination algorithm and returns the new VB_Vector. */
VB_Vector getConv(VB_Vector *inputVector, VB_Vector *inputConv, int inputSampling, int tmpResolve)
{
  // Sampling is the time resolution in HRF 
  const unsigned int expFactor = (const unsigned int) (inputSampling / tmpResolve);
  VB_Vector *convVector = new VB_Vector(inputConv);
  convVector->sincInterpolation(expFactor);
  VB_Vector oldConvVec(convVector);

  int tmpLength = inputVector->getLength();
  convVector->resize(tmpLength);
  convVector->setAll(0);
  int orgLength = oldConvVec.getLength();
  // Make sure inputVector doesn't have less elements than inputConv
  if (orgLength > tmpLength) {
    printf("getConv() error: inputConv has more elements than inputVector, convolution not allowed\n");
    return convVector;
  }

  for (int i = 0; i < orgLength; i++)
    (*convVector)[i] = oldConvVec[i];

  convVector->meanCenter();
  convVector->normMag();

  return fftConv(inputVector, convVector, true);
}

/* fftConv() is another generic function for convolution calculation. 
 * It accepts two input vb_vector arguments and a "zeroFlag", which determines
 * whether this sentence will be added:
      realKernel->setElement(0, 1.0);
 * It returns the convolution vector. 
 * Note: 
 * When it is called by getConv() to modify a single covariate and efficiency
 * checking, zeroFlag = 1;
 * But when it is called by "Fourier set" or "Eigenvector set", zeroFlag = 1 */
VB_Vector fftConv(VB_Vector *inputVector, VB_Vector *convVector, bool zeroFlag)
{
  int tmpLength = inputVector->getLength();
  VB_Vector *realKernel = new VB_Vector(tmpLength);
  VB_Vector *imagKernel = new VB_Vector(tmpLength);
  convVector->fft(realKernel, imagKernel);

  if (zeroFlag) {
    realKernel->setElement(0, 1.0);
    imagKernel->setElement(0, 0.0);
  }

  VB_Vector *orgReal = new VB_Vector(tmpLength);
  VB_Vector *orgImag = new VB_Vector(tmpLength);
  inputVector->fft(orgReal, orgImag);

  VB_Vector *newReal = new VB_Vector(tmpLength);
  VB_Vector *newImag = new VB_Vector(tmpLength);
  double tmp1 = 0, tmp2 = 0;
  for (int i = 0; i < tmpLength; i++) {
    tmp1 = realKernel->getElement(i) * orgReal->getElement(i) - 
      imagKernel->getElement(i) * orgImag->getElement(i);
    tmp2 = imagKernel->getElement(i) * orgReal->getElement(i) + 
      realKernel->getElement(i) * orgImag->getElement(i);
    newReal->setElement(i, tmp1);
    newImag->setElement(i, tmp2);
  }
  
  VB_Vector *inverseReal1 = new VB_Vector(tmpLength);
  VB_Vector *inverseImag1 = new VB_Vector(tmpLength);
  VB_Vector *inverseReal2 = new VB_Vector(tmpLength);
  VB_Vector *inverseImag2 = new VB_Vector(tmpLength);

  newReal->ifft(inverseReal1, inverseImag1);
  newImag->ifft(inverseReal2, inverseImag2);

  VB_Vector newVec = VB_Vector(tmpLength);
  for(int i = 0; i < tmpLength; i++) {
    double tmp = inverseReal1->getElement(i) - inverseImag2->getElement(i);
    newVec.setElement(i, tmp);
  }

  delete realKernel;
  delete imagKernel;
  delete orgReal;
  delete orgImag;
  delete newReal;
  delete newImag;
  delete inverseReal1;
  delete inverseImag1;
  delete inverseReal2;
  delete inverseImag2;

  return newVec;
}

/* getDeterm() returns the determinant of the input G matrix 
 * Note: What it actually returns is the determinant of:
 *          inputG * Trnaspose(inputG)
 * DO NOT use this function to calculate determinant of a square matrix */
double getDeterm(VBMatrix &inputMat)
{
  int totalReps = inputMat.m;
  int colNum = inputMat.n;

  // a is gsl_matrix copied from inputMat
  gsl_matrix *a = gsl_matrix_calloc(totalReps, colNum);
  gsl_matrix_memcpy(a, &(inputMat.mview).matrix);

  // aTa: transpose(a) * a
  gsl_matrix *aTa = gsl_matrix_calloc(colNum, colNum);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, a, a, 0.0, aTa);

  // Copied from gsl documentation (Chapter 13: Linear Algebra)
  int s;
  gsl_permutation *p = gsl_permutation_alloc(colNum);
  gsl_linalg_LU_decomp(aTa, p, &s); 
  double determ = gsl_linalg_LU_det(aTa, s);

  // time to free up the space
  gsl_permutation_free(p);
  gsl_matrix_free(a);
  gsl_matrix_free(aTa);

  return determ;
}

// calcColinear() calculates the multiple rsquared for a dependent
// variable and a set of independent variables, and returns the square
// root of that (R)

double
calcColinear(VBMatrix &ivs,VB_Vector &dv)
{
  // Make sure dv isn't a constant vector
  if (!dv.getVariance()) {
    printf("[E] calcColinear(): no colinearity calculated because the dependent parameter is constant.\n");
    return -1;
  }

  int vecLen = dv.getLength();
  VB_Vector fits=calcfits(ivs,dv);
  if (fits.size()<1) {
    printf("[E] calcColinear(): no colinearity calculated because the design matrix is singular.\n");
    return -1;
  }
  bool f_hasintercept=0;
  for (size_t i=0; i<ivs.cols; i++) {
    VB_Vector tmpv=ivs.GetColumn(i);
    if (tmpv.getVariance()>FLT_MIN) continue;
    if (abs(tmpv.getVectorMean())>FLT_MIN) f_hasintercept=1;
  }

  // special case: if there's no intercept, rmul is sum squared fits /
  // sum squared observations
  if (!f_hasintercept) {
    VB_Vector fsq=fits;
    fsq*=fits;
    VB_Vector dsq=dv;
    dsq*=dv;
    double rsquared=fsq.getVectorSum()/dsq.getVectorSum();
    return sqrt(rsquared);
  }

  VB_Vector residuals(vecLen);
  for (int i = 0; i < vecLen; i++)
    residuals[i] = dv[i] - fits.getElement(i);

  double SSerr = 0;
  for (int k = 0; k < vecLen; k++)
    SSerr += (residuals[k] * residuals[k]);

  double SStot=(dv.getVariance()) * (double)(vecLen - 1);
  double rsquared = 1.0 - (SSerr / SStot);
  if (rsquared < 0-FLT_MIN) {
    printf("calcColinear: invalid colinearity value %f.\n",rsquared);
    return -1;
  }
  else if (rsquared < 0)
    rsquared=0;
  return sqrt(rsquared);
}

// calcfits() does:
// fits=inMat ## ( invert( transpose(inMat) ## inMat ) ## transpose(inMat) ## inVecMat )

VB_Vector
calcfits(VBMatrix &inMat, VB_Vector &inVec)
{
  VBMatrix tmp1,tmp2;
  tmp1=inMat;
  tmp2=inMat;
  tmp1.transposed=1;
  tmp2^=tmp1;
  if (invert(tmp2,tmp2))
    return VB_Vector();
  tmp1=inMat;
  tmp1.transposed=1;
  tmp2*=tmp1;
  tmp1=VBMatrix(inVec);
  tmp2*=tmp1;
  tmp2^=inMat;
  return tmp2.GetColumn(0);
}


/* This method is to count the number of elements in 
 * a VB_Vector that equal to a certain value m */
int countNum(VB_Vector *inputVector, int m)
{
  int length = inputVector->getLength();
  int count = 0;
  int element;
  for (int i = 0; i < length; i++) {
    element = (int) inputVector->getElement(i);
    if (m == element)
      count++;
  }

  return count;
}

/* This function will count the number of non-zero elements in 
 * an input vector, Written for "Mean center non-zero" functionality */
int countNonZero(VB_Vector *inputVector)
{
  int length = inputVector->getLength();
  int count = 0;
  double element;
  for (int i = 0; i < length; i++) {
    element = inputVector->getElement(i);
    if (element)
      count++;
  }

  return count;  
}

/* This function accepts an input vb_vector and converts it by delta calculation.
 * It follows Geoff's G_Delta procedures. 
 * Note that although VB_Vector class has left shift (operator<<) and right shift (operator>>)
 * defined, they are different from IDL's shift function because shift() also wraps the elements
 * at the end (for right shift) or beginning (for left shift). */
void calcDelta(VB_Vector *inputVec)
{
  int vecLength = inputVec->getLength();
  VB_Vector *tmpVec = new VB_Vector(inputVec);
  VB_Vector *shifter = new VB_Vector(vecLength); 
  inputVec->setAll(0.0);

  // calculate shifter   
  (*shifter)[0] = (*tmpVec)[0] - (*tmpVec)[vecLength - 1];
  for (int i = 1; i < vecLength; i++) {
      (*shifter)[i] = (*tmpVec)[i] - (*tmpVec)[i - 1];
  }

  for (int i = 0; i < vecLength; i++) {
    /* Different from Geoff here: Instead of using (*shifter)[i] > 0, another criteria is used.
     * With this standard, it might be better to distinguish between the real element difference
     * and fluctuation from noise. */
    if ((*shifter)[i] > fabs(tmpVec->getMaxElement()) * 0.00001) { 
      (*inputVec)[i] = 1;
    }
  }

  double vecSum = tmpVec->getVectorSum();
  double stdDev = sqrt(tmpVec->getVariance());

  if ((vecSum > -0.9) && (vecSum < 0.9))
    inputVec->meanCenter();

  if ((stdDev > 0.9) && (stdDev < 1.1))
    inputVec->unitVariance();

  delete tmpVec;
  delete shifter;
}

/* getTxtColNum() returns the number of elements 
 * on the first uncommented line from input file 
 * returns -1 if it is not readable */
int getTxtColNum(const char *inputFile)
{
  FILE *fp;
  char line[1024]; // Assume each line has 1024 characters or less
  fp = fopen(inputFile,"r");
  if (!fp)
    return -1;     // Returns 1 if file not readable

  int colNum = 0;
  tokenlist myRow;
  while (fgets(line,1024,fp)) {
    if (strchr(";#%\n",line[0]))
      continue;
    stripchars(line,"\n");
    const string tmpStr(line);
    myRow = tokenlist(tmpStr);
    colNum = myRow.size();
    myRow.clear();
    break;
  }

  fclose(fp);
  return colNum;
}

/* getTxtRowNum() returns the number of rows from input file 
 * returns -1 if it is not readable */
int getTxtRowNum(const char *inputFile)
{
  FILE *fp;
  char line[1024]; // Assume each line has 1024 charactoers or less
  fp = fopen(inputFile,"r");
  if (!fp)
    return -1;    // returns 1 if file not readable

  int rowNum = 0;
  while (fgets(line,1024,fp)) {
    if (strchr(";#%\n",line[0]))
      continue;
    rowNum++;
  }
  fclose(fp);
  return rowNum;
}

/* readTxt() checks the input file's format. 
 * If it is valid, read each column into the vb_vector array txtCov;
 * returns 0 when the input file is good; returns 1 when the input file 
 * doesn't have exactly the same number of elements in a row */
int readTxt(const char *inputFile, std::vector< VB_Vector *> txtCov)
{
  FILE *fp;
  char line[1024]; // Assume each line has 1024 charactoers or less

  fp = fopen(inputFile,"r");
  size_t currentRowNum = 0;
  size_t colNum = 0;
  tokenlist myRow;
  while (fgets(line,1024,fp)) {
    if (strchr(";#%\n",line[0]))
      continue;
    stripchars(line,"\n");
    const string tmpStr(line);
    myRow = tokenlist(tmpStr);
    if (currentRowNum == 0)
      colNum = myRow.size();
    if (colNum != myRow.size()) {
      fclose(fp);
      return 1;
    }
    for (size_t j = 0; j < colNum; j++)
      txtCov[j]->setElement(currentRowNum, atof(myRow(j)));
    currentRowNum++;
    myRow.clear();
  }

  fclose(fp);
  return 0;    // no error!
}

/* derivative() is a function that simulates Geoff's derivative() function in IDL 
 * Reference: VoxBo_Fourier.pro */
VB_Vector * derivative(VB_Vector *inputVec)
{
  unsigned vecLength = inputVec->getLength();
  if (vecLength % 2 != 0) {
    printf("Error in derivative(): odd number of elements in input vector: %d\n", vecLength);
    return 0;
  }

  VB_Vector *fftReal = new VB_Vector(vecLength);
  VB_Vector *fftImg = new VB_Vector(vecLength);
  inputVec->fft(fftReal, fftImg);

  VB_Vector *fftDevReal = new VB_Vector(vecLength);
  fftDevReal->setAll(0);
  VB_Vector *fftDevImg = new VB_Vector(vecLength);
  fftDevImg->setAll(0);

  double fundFreq = 2.0 * 3.14159 / (double) vecLength;
  double freq = 0, tmpReal = 0, tmpImg = 0;
  for (unsigned h = 1; h < vecLength / 2; h++) {
    freq = fundFreq * (double) h;

    tmpReal = fftReal->getElement(h);
    tmpImg = fftImg->getElement(h);
    fftDevReal->setElement(h, tmpImg * freq * (-1.0));
    fftDevImg->setElement(h, tmpReal * freq);

    tmpReal = fftReal->getElement(vecLength - h);
    tmpImg = fftImg->getElement(vecLength - h);
    fftDevReal->setElement(vecLength - h, tmpImg * freq);
    fftDevImg->setElement(vecLength - h, tmpReal * freq * (-1.0));
  }

  VB_Vector *ifftDevRealReal = new VB_Vector(vecLength);
  VB_Vector *ifftDevRealImg = new VB_Vector(vecLength);
  VB_Vector *ifftDevImgReal = new VB_Vector(vecLength);
  VB_Vector *ifftDevImgImg = new VB_Vector(vecLength);

  fftDevReal->ifft(ifftDevRealReal, ifftDevRealImg);
  fftDevImg->ifft(ifftDevImgReal, ifftDevImgImg);

  VB_Vector *finalVec = new VB_Vector(vecLength);
  for (unsigned i = 0; i < vecLength; i++) {
    tmpReal = ifftDevRealReal->getElement(i) - ifftDevImgImg->getElement(i);
    finalVec->setElement(i, tmpReal);
  }
  delete fftReal;
  delete fftImg;
  delete fftDevReal;
  delete fftDevImg;
  delete ifftDevRealReal;
  delete ifftDevRealImg;
  delete ifftDevImgReal;
  delete ifftDevImgImg;

  return finalVec;
}

/*
VB_Vector
GLMInfo::getFits(VBRegion &rr)
{
  VB_Vector fits;
  // build a vector of VB_Vectors with all the covariates
  // for each variable that's of interest, grab the scaled version
  int xx,yy,zz;
  vector<VB_Vector>vars;
  vector<VB_Vector>indices;
  string kgname=glmi.stemname+".KG";
  string gname=glmi.stemname+".G";
  string prmname=glmi.stemname+".prm";
  VBMatrix KG;
  Tes prm;

  if (KG.ReadFile(kgname))
    if (KG.ReadFile(gname))
      return fits;
  if (prm.Readheader(prmname))
    return fits;

  int ntimepoints=KG.m;
  // VB_Vector var=KG.getColumn(interestlist[i]);
  VB_Vector tmpfits;
  tmpfits.resize(ntimepoints);
  
  for (int j=0; j<myregion.size(); j++) {
    VB_Vector vf(ntimepoints);
    xx=myregion[j].x;
    yy=myregion[j].y;
    zz=myregion[j].z;
    // get betas for this voxel
    if (prm.ReadTimeSeries(prmname,x,y,z))
      return fits;
    VB_Vector weights(nvars);
    // copy ones of interest
    for (size_t i=0; i<interestlist.size(); i++) {
      int ind=interestlist[i];
      vf+=prm.timeseries[ind]*KG.getColumn(i);
    }
  }
  return fits;
}
*/

void
GLMInfo::initthresh()
{
  string prmname=xsetextension(stemname,"prm");
  string sename=xsetextension(stemname,"se");
  string tracesname=xsetextension(stemname,"traces");
  if (!paramtes)
    paramtes.ReadFile(prmname);
  if (!paramtes)
    thresh.numVoxels=0;
  thresh.numVoxels=paramtes.realvoxels;
  thresh.searchVolume=paramtes.realvoxels*paramtes.voxsize[0]*
    paramtes.voxsize[1]*paramtes.voxsize[2];
  thresh.vsize[0]=paramtes.voxsize[0];
  thresh.vsize[1]=paramtes.voxsize[1];
  thresh.vsize[2]=paramtes.voxsize[2];
  VB_Vector se,traces;
  double effdf=0.0;
  se.ReadFile(sename);
  if (se.size()==3)
    thresh.fwhm=(se[0]+se[1]+se[2])/3.0;
  else
    thresh.fwhm=0.0;
  thresh.pValPeak=0.05;
  traces.ReadFile(tracesname);
  if (traces.size()==3)
    effdf=traces[2];
  if (contrast.scale[0]=='f') {
    int nz=0;
    for (size_t i=0; i<contrast.contrast.size(); i++)
      if (fabs(contrast.contrast[i])>FLT_MIN)
        nz++;
    thresh.effdf=effdf;
    thresh.denomdf=nz;
  }
  else
    thresh.denomdf=0,thresh.effdf=effdf;
  thresh.pValExtent=0.05;
  thresh.clusterThreshold=0.001;
}


// return the desired covariate, possibly scaled
VB_Vector
GLMInfo::getCovariate(int x,int y,int z,int paramindex,int scaledflag)
{
  VB_Vector covar;
  string prmname=xsetextension(stemname,"prm");
  string kgname=xsetextension(stemname,"KG");
  VBMatrix KG;

  int errs=0;
  int ntimepoints;
  int nbetas;
  // load KG
  KG.ReadFile(kgname);
  ntimepoints=KG.m;
  nbetas=KG.n;
  // if not valid, return the empty covariate
  if (!KG.valid())
    return covar;
  // fill the covariate with the right row of the KG matrix
  covar.resize(ntimepoints);
  for (int i=0; i<ntimepoints; i++)
    covar.setElement(i,KG(i,paramindex));
  // if scaled, get the beta
  if (scaledflag) {
    Tes prm;
    if (prm.ReadHeader(prmname))
      errs++;
    if (prm.ReadTimeSeries(prmname,x,y,z))
      errs++;
    if ((int)(prm.timeseries.getLength()) <=paramindex)
      errs++;
    if (errs==0)
      covar*=prm.timeseries[paramindex];
  }
  return covar;
}

int
GLMInfo::filterTS(VB_Vector &signal)
{
  if (!(exoFilt.getLength())) {
    string exoname=xsetextension(stemname,"ExoFilt");
    exoFilt.ReadFile(exoname);
    if (!(exoFilt.getLength()))
      return 101;
  }
  VB_Vector exo_real(exoFilt.getLength());
  VB_Vector exo_imag(exoFilt.getLength());
  VB_Vector sig_real(signal.getLength());
  VB_Vector sig_imag(signal.getLength());
  VB_Vector prod_real(signal.getLength());
  VB_Vector prod_imag(signal.getLength());
  exoFilt.fft(exo_real,exo_imag);
  exo_real[0]=1.0;
  exo_imag[0]=0.0;
  signal.fft(sig_real,sig_imag);
  VB_Vector::compMult(sig_real,sig_imag,exo_real,exo_imag,prod_real,prod_imag);
  VB_Vector::complexIFFTReal(prod_real,prod_imag,signal);
  return 0;
}

int
GLMInfo::makeF1()
{
  // see if F1 is already populated
  if (f1Matrix.m) return 0;
  // see if it's on disk
  string f1name=xsetextension(stemname,"F1");
  f1Matrix.ReadFile(f1name);
  if (f1Matrix.m) return 0;
  // see if we can load a KG to pinv
  string kgname=xsetextension(stemname,"KG");
  VBMatrix kg;
  kg.ReadFile(kgname);
  if (kg.m) {
    f1Matrix.init(kg.n,kg.m);
    if (pinv(kg,f1Matrix))
      return 2;
    return 0;
  }
  // see if we have or can load a G matrix to pinv
  if (!gMatrix.m) {
    string gname=xsetextension(stemname,"G");
    gMatrix.ReadFile(gname);
  }
  if (gMatrix.m) {
    f1Matrix.init(gMatrix.n,gMatrix.m);
    if (pinv(gMatrix,f1Matrix))
      return 4;
    return 0;
  }
  return 1;  // give up, return error
}

// adjustTS() adjusts for covariates of no interest by first using F1
// with the (possibly spatially averaged) time series to calculate the
// betas.  the time series should already have been filtered.

int
GLMInfo::adjustTS(VB_Vector &signal)
{
  string kgname=xsetextension(stemname,"KG");
  string gname=xsetextension(stemname,"G");
  VBMatrix KG;

  if (makeF1())
    return 190;
  KG.ReadFile(kgname);
  if (!KG.m)
    KG.ReadFile(gname);
  if (!KG.m)
    return 191;

  // grab betas, kg, and g matrix figure out which covariates are not
  // of interest, scale them, and subtract them from the signal.  note
  // that for non-autocorrelated designs, KG is just the G matrix

  // for convenience
  int ntimepoints=f1Matrix.n;
  int nvars=f1Matrix.m;
  // calculate betas
  VB_Vector betas(nvars);
  for (int i=0; i<nvars; i++) {
    betas[i]=0;
    for (int j=0; j<ntimepoints; j++) {
      betas[i] += f1Matrix(i, j) * signal[j];
    }
  }

  for (size_t i=0; i<nointerestlist.size(); i++) {
    VB_Vector tmp(ntimepoints);
    for (int j=0; j<ntimepoints; j++)
      tmp.setElement(j,KG(j,nointerestlist[i])*betas[nointerestlist[i]]);
    signal-=tmp;
  }
  return 0;
}

VB_Vector
GLMInfo::getTS(int x,int y,int z,uint32 flags)
{
  return ::getTS(teslist,x,y,z,flags);
}

VBRegion
GLMInfo::restrictRegion(VBRegion &rr)
{
  VBRegion newreg;
  loadcombinedmask();
  // add masked-in voxels to new region
  for (VI myvox=rr.begin(); myvox!=rr.end(); myvox++) {
    if (mask.testValue(myvox->second.x,myvox->second.y,myvox->second.z))
      newreg.add(myvox->second);
  }
  return newreg;
}

VB_Vector
GLMInfo::getRegionTS(VBRegion &rr,uint32 flags)
{
  return ::getRegionTS(teslist,rr,flags);
}

VBMatrix
GLMInfo::getRegionComponents(VBRegion &rr,uint32 flags)
{
  return ::getRegionComponents(teslist,rr,flags);
}

// residual is R * transpose(filtered_time_series)

VB_Vector
GLMInfo::getResid(int x,int y,int z,uint32 flags)
{
  VBRegion rr;
  rr.add(x,y,z,0.0);
  return getResid(rr,flags);
}

VB_Vector
GLMInfo::getResid(VBRegion &rr,uint32 flags)
{
  VB_Vector resid;
  if (rMatrix.m==0)
    rMatrix.ReadFile(xsetextension(stemname,"R"));
//   if (!paramtes.data_valid)
//     paramtes.ReadFile(xsetextension(stemname,"prm"));
  if (exoFilt.size()==0)
    exoFilt.ReadFile(xsetextension(stemname,"ExoFilt"));
  if (rMatrix.m==0 || exoFilt.size()==0)
    return resid;

  // assemble signal vector
  VB_Vector signal=getRegionTS(rr,flags);
  int ntimepoints=signal.getLength();

  // filter signal matrix
  VB_Vector exo_real(exoFilt.getLength());
  VB_Vector exo_imag(exoFilt.getLength());
  VB_Vector sig_real(signal.getLength());
  VB_Vector sig_imag(signal.getLength());
  VB_Vector prod_real(signal.getLength());
  VB_Vector prod_imag(signal.getLength());
  exoFilt.fft(exo_real,exo_imag);
  exo_real[0]=1.0;
  exo_imag[0]=0.0;
  signal.fft(sig_real,sig_imag);
  VB_Vector::compMult(sig_real,sig_imag,exo_real,exo_imag,prod_real,prod_imag);
  VB_Vector::complexIFFTReal(prod_real,prod_imag,signal);
  
  // premult KX (filtered data) by residual forming matrix R
  resid.resize(ntimepoints);
  gsl_blas_dgemv(CblasNoTrans,1.0,&(rMatrix.mview.matrix),
                 signal.getTheVector(),0.0,resid.getTheVector());
  return resid;
  
}

GLMInfo::GLMInfo()
{
  init();
}

void
GLMInfo::setup(string name)
{
  init();
  findstem(name);
  findanatomy();
  findtesfiles();
  getcovariatenames();
  loadcontrasts();
  loadtrialsets();
  getglmflags();
}

void
GLMInfo::init()
{
  stemname="";
  anatomyname="";
  teslist.clear();
  cnames.clear();
  contrasts.clear();
  trialsets.clear();
  nvars=0;
  dependentindex=0;
  interceptindex=0;
  glmflags=0;
  gMatrix.clear();
  f1Matrix.clear();
  rMatrix.clear();
  f3Matrix.clear();
  exoFilt.clear();
  residuals.clear();
  betas.clear();
  traceRV.clear();
  pseudoT.clear();
  keeperlist.clear();
  interestlist.clear();
  paramtes.init();
  statcube.init();
  rawcube.init();
  realExokernel.clear();
  imagExokernel.clear();
  effdf=-1;
}

void
GLMInfo::findstem(string name)
{
  struct stat st;
  // name exists
  if (!(stat(name.c_str(),&st))) {
    if (S_ISDIR(st.st_mode)) {
      vglob vg(name+"/*.glm");
      if (vg.size())
        stemname=xsetextension(vg.names[0],"");
      else
        stemname=name+"/"+xfilename(name);
      return;
    }
    else {
      stemname=xdirname(name)+"/"+xsetextension(xfilename(name),"");
      return;
    }
  }
  stemname=name;
  return;
}

void
GLMInfo::findanatomy()
{
  string mydir=xdirname(stemname);
  string parentdir=xdirname(mydir);
  
  vglob vg;
  vg.append(mydir+"/[Dd]isplay.*");
  vg.append(mydir+"/[Aa]natomical.*");
  vg.append(mydir+"/[Aa]nat.*");
  vg.append(parentdir+"/[Aa]natomy/[Dd]isplay.*");
  vg.append(parentdir+"/[Aa]natomy/[Aa]natomical.*");
  vg.append(parentdir+"/[Aa]natomy/[Aa]nat.*");
  for (size_t i=0; i<vg.size(); i++) {
    Cube cb;
    if (cb.ReadHeader(vg.names[i])==0) {
      anatomyname=vg.names[i];
      return;
    }
  }
  return;
}

void
GLMInfo::print()
{
  printf("          stem: %s\n",stemname.c_str());
  printf("       anatomy: %s\n",anatomyname.c_str());
  printf("     tes files: %d\n",(int)teslist.size());
  printf("     dependent: %d\n",dependentindex);
  printf("  n indep vars: %d\n",nvars);
  printf("   vars of int: %d\n",(int)interestlist.size());
  printf("    covariates: ");
  if (cnames.size())
    printf("%c %s\n",cnames[0][0],cnames[0].c_str()+1);
  else
    printf("<none>\n");
  for (size_t i=1; i<cnames.size(); i++)
    printf("                %c %s\n",cnames[i][0],cnames[i].c_str()+1);
  printf("     contrasts: ");
  if (contrasts.size())
    printf("%s (%s)\n",contrasts[0].name.c_str(),contrasts[0].scale.c_str());
  else
    printf("<none>\n");
  for (size_t i=1; i<contrasts.size(); i++)
    printf("                %s (%s)\n",contrasts[i].name.c_str(),contrasts[i].scale.c_str());
}

string
GLMInfo::statmapExists(string matrixStemName, VB_Vector& contrasts, string scale) {
  string clist, plist;
  char temp[STRINGLEN];
  for (size_t i=0; i<contrasts.size(); i++) {
      sprintf(temp, "%.0f", contrasts[i]);
      clist+=temp;
      clist+=(" ");
  }
  Tes prm(matrixStemName+"/"+matrixStemName+".prm");
  string prmtimestamp = prm.GetHeader("TimeStamp:");
  Cube cub;
  string list=matrixStemName+"/map_*.cub";
  vglob vg(list);
  for (size_t i=0; i<vg.size(); i++) {
    cub.ReadFile(vg[i]);
    if (cub.GetHeader("contrast_scale:")==(scale) &&
        cub.GetHeader("contrast_vector:")==(clist) &&
        cub.GetHeader("TimeStamp:")==(prmtimestamp)){
          return string(vg[i]);
    }
  }
  return "";
}

void
GLMInfo::findtesfiles()
{
  ifstream subfile;
  tokenlist line;
  subfile.open((stemname+".sub").c_str());
  if (!subfile)
    return;
  char subline[STRINGLEN];
  while (subfile.getline(subline,STRINGLEN,'\n')) {
    line.ParseLine(subline);
    if (line.size()==0)
      continue;
    if (line[0][0]==';' || line[0][0]=='#')
      continue;
    if (line[0]=="VB98" || line[0]=="TXT1")
      continue;
    teslist.push_back(line[0]);
  }
  subfile.close();
}

void
GLMInfo::loadcombinedmask()
{
  if (mask.data)
    return;
  mask.init();
  tesgroup.resize(teslist.size());
  for (size_t i=0; i<teslist.size(); i++) {
    if (tesgroup[i].ReadHeader(teslist[i])) {
      mask.init();
      return;
    }
    if (!tesgroup[i].fileformat.f_headermask){
      if (tesgroup[i].ReadFile(teslist[i])) {
        mask.init();
        return;
      }
    }
    Cube tesmask;
    if (tesgroup[i].ExtractMask(tesmask))
      continue;
    if (!(mask.data)) {
      mask=tesmask;
    }
    else
      mask.intersect(tesmask);
  }
}

void
GLMInfo::getcovariatenames()
{
  dependentindex=-1;
  interceptindex=-1;
  string gname=stemname+".G";
  int index;
  VBMatrix m(gname);
  tokenlist line;
  line.SetSeparator("\t");
  string tag,type,name;
  keeperlist.clear();
  interestlist.clear();
  nointerestlist.clear();
  nvars=0;
  for (size_t i=0; i<m.header.size(); i++) {
    line.ParseLine(m.header[i]);
    tag=line[0];
    index=strtol(line[1]);
    type=line[2];
    tag=vb_tolower(tag);
    type=vb_tolower(type);
    name=vb_tolower(line[3]);  // FIXME only gets the first token in the name
    if (tag!="parameter:")
      continue;
    nvars++;
    if (type=="interest")
      cnames.push_back((string)"I"+line[3]);
    else if (type=="nointerest")
      cnames.push_back((string)"N"+line[3]);
    else if (type=="keepnointerest")
      cnames.push_back((string)"K"+line[3]);
    else if (type=="dependent")
      cnames.push_back((string)"D"+line[3]);
    else
      cnames.push_back((string)"U"+line[3]);
    if (type=="interest" || type=="keepnointerest")
      keeperlist.push_back(index);
    if (type=="interest")
      interestlist.push_back(index);
    if (type=="keepnointerest" || type=="nointerest")
      nointerestlist.push_back(index);
    if (type=="dependent")
      dependentindex=index;
    if (name=="intercept") {
      interceptindex=index;
    }
  }
}

void
GLMInfo::loadtrialsets()
{
  string fname=xdirname(stemname)+"/averages.txt";
  trialsets=parseTAFile(fname);
}

void
GLMInfo::loadcontrasts()
{
  contrasts.clear();
  ifstream contfile;
  tokenlist line,args;
  VBMatrix gmatrix;

  gmatrix.ReadHeader(stemname+".G");
  if (nvars==0) {
    for (size_t i=0; i<gmatrix.header.size(); i++) {
      args.ParseLine(gmatrix.header[i]);
      if (args[0]=="Parameter:")
        nvars++;
    }
  }
  if (nvars<1)
    return;

  vector<string> cfnames;
  cfnames.push_back(xdirname(stemname)+"/contrasts.txt");
  cfnames.push_back(xdirname(stemname)+"/contrast.txt");
  cfnames.push_back(stemname+".contrasts");
  cfnames.push_back(stemname+".contrast");
  
  for (size_t i=0; i<cfnames.size(); i++) {
    contfile.open(cfnames[i].c_str());
    if (contfile) {
      char cline[STRINGLEN];
      while (contfile.getline(cline,STRINGLEN,'\n')) {
        line.ParseLine(cline);
        if (line.size()==0)
          continue; 
        if (line[0][0]==';' || line[0][0]=='#')
          continue;
        if (line[0]=="VB98" || line[0]=="TXT1")
          continue;
        if (line.size()<3)
          continue;
        VBContrast ss;
        if (ss.parsemacro(line,nvars,interestlist)==0)
          contrasts.push_back(ss);
      }
      contfile.close();
    }
  }
  if (contrasts.size()==0 && nvars>0) {
    VBContrast ss;
    tokenlist tmpt;
    tmpt.ParseLine("all t allspikes");
    ss.parsemacro(tmpt,nvars,interestlist);
    contrasts.push_back(ss);
    tmpt.ParseLine("first t spikes 0");
    ss.parsemacro(tmpt,nvars,interestlist);
    contrasts.push_back(ss);
  }
}

void
GLMInfo::getglmflags()
{
  glmflags=0;
  // first scour tes header for indications that the data should be
  // detrended and/or mean scaled (these are done on the fly, not to
  // the tes data directly)
  Tes tmp;
  tokenlist args;
  if (tmp.ReadHeader(stemname+".prm")==0) {
    for (size_t i=0; i<tmp.header.size(); i++) {
      args.ParseLine(tmp.header[i]);
      string tag=vb_tolower(xstripwhitespace(args[0]," \t\n:"));
      if (tag=="options" || tag=="option") {
        for (size_t j=1; j<args.size(); j++) {
          if (vb_tolower(args[j])=="detrend")
            glmflags|=DETREND;
          else if (vb_tolower(args[j])=="meanscale")
            glmflags|=MEANSCALE;
        }
      }
      else if (tag=="datascale") {
        if (vb_tolower(args[1])=="mean")
          glmflags|=MEANSCALE;
      }
    }
  }
  // next, check if autocorrelated
  if (vb_fileexists(stemname+".ExoFilt") ||
      vb_fileexists(stemname+".IntrinCor"))
    glmflags|=AUTOCOR;
}

int
GLMInfo::parsecontrast(const string &str)
{
  // first try to look it up
  for (size_t i=0; i<contrasts.size(); i++) {
    if (vb_tolower(contrasts[i].name)==vb_tolower(str)) {
      contrast=contrasts[i];
      return 0;  // no error!
    }
  }
  // try to parse it as a contrast macro
  tokenlist line;
  line.ParseLine(str);
  if (!(contrast.parsemacro(line,nvars,interestlist)))
    return 0;  // no error!
  // try to parse it as a series of numbers
  contrast.name="mycontrast";
  contrast.scale="t";
  contrast.contrast.resize(nvars);
  for (int i=0; i<nvars; i++) contrast.contrast[i]=0.0;
  if (line.size()<1)
    return 101;
  if (validscale(line[0])) {
    contrast.scale=line[0];
    line.DeleteFirst();
  }
  if (line.size()!=interestlist.size())
    return 102;
  for (size_t i=0; i<line.size(); i++) {
    if (!isdigit(line[i][0]) && !(strchr("-.",line[i][0])))
      return 102;
    contrast.contrast[interestlist[i]]=strtod(line[i]);
  }
  return 0;
}

int
VBContrast::parsemacro(tokenlist &line,int nvars,vector<int> &interestlist)
{
  if (nvars<1)
    return 102;
  name=line[0];
  scale=line[1];
  contrast.resize(nvars);
  if (line[2]=="allspikes") {
    contrast+=1.0;
  }
  else if (line[2]=="spikes" || line[2]=="spike") {
    vector<int> myspikes=numberlist(line[3]);
    for (size_t i=0; i<myspikes.size(); i++) {
      if (myspikes[i]>(int)interestlist.size()-1)
        return 109;
      contrast[interestlist[myspikes[i]]]=1.0;
    }
  }
  else if (line[2]=="vec" && line.size()-3==interestlist.size()) {
    if (line.size()-3!=interestlist.size())
      return 105;
    for (size_t i=3; i<line.size(); i++)
      contrast[interestlist[i-3]]=strtod(line[i]);
  }
  else if (line[2]=="contrast") {
    // FIXME used at all?  validated?
    vector<int> myspikes=numberlist(line[3]);
    for (size_t i=0; i<myspikes.size(); i++)
      contrast[interestlist[myspikes[i]]]=1;
    if (line[4]=="minus")
      myspikes=numberlist(line[5]);
    else
      myspikes=numberlist(line[4]);
    for (size_t i=0; i<myspikes.size(); i++)
      contrast[interestlist[myspikes[i]]]=-1;
  }
  else
    return 101;

  return 0;  // no error!
}

void
VBContrast::print()
{
  printf("[I] contrast %s (%s):",name.c_str(),scale.c_str());
  for (size_t i=0; i<contrast.size(); i++)
    printf(" %.1f",contrast[i]);
  printf("\n");
}

int
validscale(string scale) {
  scale=xstripwhitespace(vb_tolower(scale));
  if (scale=="t"||scale=="f"|| scale=="tp"||scale=="fp"||scale=="tz"||scale=="fz")
    return 1;
  if (scale=="beta"||scale=="rawbeta"||scale=="rb"||scale=="b")
    return 1;
  if (scale=="intercept"||scale=="int"||scale=="i"||scale=="pct"||scale=="percent")
    return 1;
  if (scale=="tp"||scale=="fp"||scale=="tz"||scale=="fz")
    return 1;
  if (scale=="tp/1"||scale=="tp/2"||scale=="tp1"||scale=="tp2")
    return 1;
  if (scale=="tz/1"||scale=="tz/2"||scale=="tz1"||scale=="tz2")
    return 1;
  if (scale=="error"||scale=="err"||scale=="e")
    return 1;
  return 0;
}

// permutes the passed vector

void
GLMInfo::permute_if_needed(VB_Vector &vec)
{
  if (perm_signs.size()==vec.size()) {
    for (size_t i=0; i<vec.size(); i++)
      vec[i]*=perm_signs[i];
  }
  if (perm_order.size()==vec.size()) {
    VB_Vector tmp(vec.size());
    for (size_t i=0; i<vec.size(); i++)
      tmp[i]=vec[(int)perm_order[i]];
    vec=tmp;
  }
}
