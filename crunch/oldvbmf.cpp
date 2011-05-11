
// vbmakefilter.cpp
// hack to create exofilt file
// Copyright (c) 2005 by The VoxBo Development Team

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
#include <sys/wait.h>

VBPrefs vbp;

class FilterParams {
public:
  string exofiltname;
  string noisename;
  int lows,highs;           // low and high frequencies to remove
  vector<int> middles;      // middle frequencies to remove
  string kernelname;        // temporal smoothing kernel
  double kerneltr;          // sampling interval of kernel file
  string noisemodel;        // noise model
  int pri;
  double TR;
  int orderg;
  int init(tokenlist &args);
  int MakeNoise();
  int MakeExoFilt();
};

void MCopy(VB_Vector &c,Vec &vec);
void MCopy(Vec &vec,VB_Vector &c);
void vbmakefilter_help();
void vbmakefilter_version();

int
main(int argc,char **argv)
{
  tokenlist args;

  args.Transfer(argc-1,argv+1);
  if (args.size() == 0) {
    vbmakefilter_help();
    exit(0);
  }
  if (args[0]=="-h") {
    vbmakefilter_help();
    exit(0);
  }
  if (args[0]=="-v") {
    vbmakefilter_version();
  }

  FilterParams fp;
  if (fp.init(args))
    exit(5);
  if (fp.MakeNoise())
    exit(6);
  if (fp.MakeExoFilt())
    exit(7);
  exit(0);
}

// this is the sole constructor, it chews on the passed arguments

int
FilterParams::init(tokenlist &args)
{
  exofiltname="";
  noisename="";
  lows=highs=0;
  middles.clear();
  kernelname="";
  kerneltr=2000.0;
  noisemodel="";
  pri=2;
  TR=2000.0;
  orderg=100;
  for (int i=0; i<args.size(); i++) {
    if (args[i]=="-i" && i<args.size()-1) {
      noisename=args[++i];
    }
    else if (args[i]=="-e" && i<args.size()-1) {
      exofiltname=args[++i];
    }
    else if (args[i]=="-t" && i<args.size()-2) {
      orderg=strtol(args[++i]);
      TR=strtod(args[++i]);
    }
    else if (args[i]=="-lf" && i<args.size()-1) {
      lows=strtol(args[++i]);
    }
    else if (args[i]=="-mf" && i<args.size()-1) {
      middles=numberlist(args[++i]);
    }
    else if (args[i]=="-hf" && i<args.size()-1) {
      highs=strtol(args[++i]);
    }
    else if (args[i]=="-k" && i<args.size()-1) {
      kernelname=args[++i];
    }
    else if (args[i]=="-n" && i<args.size()-1) {
      noisemodel=args[++i];
    }
    else {
      printf("[E] vbmakefilter: error processing arguments\n");
      return 101;
    }
  }
  return 0;
}

int
FilterParams::MakeNoise()
{
  if (!noisename.size())  // need output filename
    return 0;
  int i;
  double TRs=(double)TR/1000.0;

  // if no noisemodel file specified, we assume uncolored noise
  if (!noisemodel.size()) {
    Vec tmpv(orderg);
    tmpv(0)=1.0;
    for (i=1; i<orderg; i++)
      tmpv(i)=0.0;
    tmpv.AddHeader("[I] vbmakefilter: no noise model specified, assuming uncolored noise");
    tmpv.WriteFile(noisename.c_str());
    return 0;    // error == 0 is good 
  }

  VB_Vector x(orderg/2);
  Vec p(noisemodel.c_str());
  if (p.size() < 3) {
    printf("[E] vbmakefilter: ill-formed 1/f spec in %s\n",noisemodel.c_str());
    return 101;
  }
  for (i=0; i<(int)x.getLength(); i++)
    x(i)=((double)i+1) / (double) (TRs*orderg);
  VB_Vector halff(orderg/2);
  for (i=0; i<(int)halff.getLength(); i++)
    halff[i]=(1.0/(p(0)*(x(i)+p(2))))+p(1);

  // now make full array
  VB_Vector f(orderg);
  for (i=0; i<(int)halff.getLength(); i++)
    f[i+1]=halff(i);
  for (i=1; i<orderg/2; i++)
    f[orderg-i]=halff(i);
  if (halff.getLength() % 2)    // if it's odd, duplicate the middle one?
    f[i]=halff(i-1);
  f[0]=0.0;
  double fmax=f.getMaxElement();
  if (fmax == 0.0) {
    printf("[E] vbmakefilter: ill-formed filter\n");
    return 102;
  }
  for (int i=0; i<(int)f.getLength(); i++)
    f(i)=f(i)/fmax;
  VB_Vector ff(f.getLength()),imagp(f.getLength());
  f.ifft(ff,imagp);
  ff.normMag();

  // write it out
  Vec tmpv(ff.getLength());
  MCopy(tmpv,ff);
  tmpv.AddHeader((string)"noisemodel: "+noisemodel);
  if (tmpv.WriteFile(noisename.c_str())) {
    printf("[E] vbmakefilter: error writing noise model %s\n",noisename.c_str());
    return 105;
  }
  printf("[I] vbmakefilter: wrote noise model %s\n",noisename.c_str());
  return 0;    // error == 0 is good
}

int
FilterParams::MakeExoFilt()
{
  if (!exofiltname.size())
    return 0;
  double total=0;
  VB_Vector exofilt(orderg);
  int i;
  VB_Vector IRF;

  if (kernelname.size()) {
    Vec xxx;
    xxx.ReadFile(kernelname.c_str());
    string trline=GetHeader(xxx.header,"TR(msecs)");
    if (trline.size()) {
      tokenlist tmpl;
      tmpl.ParseLine(trline);
      if (tmpl.size()>1)
        kerneltr=strtod(tmpl[1]);
    }
    printf("[I] vbmakefilter: using kernel TR of %.2f\n",kerneltr);
  }
  printf("[I] vbmakefilter: using data TR of %.2f\n",TR);

  for (i=0; i<orderg; i++)
    exofilt(i)=0.0;
  exofilt(0)=1.0;

  if (kernelname.size() > 0) {
    if (IRF.ReadFile(kernelname.c_str())) {
      printf("[E] vbmakefilter: couldn't read kernel file %s\n",kernelname.c_str());
      return 101;   // error!
    }
  }

  // interpolate the HRF to the resolution of your data
  if (kernelname.size()>0 && TR != kerneltr) {
    IRF=cspline_resize(IRF,kerneltr/TR);
  }

  // set exofilt to irf padded with zeros, scaled
  if (IRF.getLength()) {
    total=0.0;
    for (i=0; i<(int)IRF.getLength(); i++) {
      exofilt[i]=IRF[i];
      total+=IRF[i];
    }
    total /= orderg;
    for (i=0; i<orderg; i++)
      exofilt(i) -= total;
  }

  // do the notch filter
  int ind;
  if (lows || highs) {
    VB_Vector realp(orderg),imagp(orderg);
    exofilt.fft(realp,imagp);
    realp(0)=imagp(0)=0.0;
    if (lows) {
      for (i=1; i<=lows; i++) {
        realp(i)=imagp(i)=0.0;
        ind=orderg-(i+1);
        realp(ind)=imagp(ind)=0.0;
      }
      realp(orderg-1)=imagp(orderg-1)=0.0;
    }
    for (int m = 1; m < (int)middles.size(); m++) {
      realp(middles[m])=imagp(middles[m])=0.0;
      ind=orderg-(middles[m]+1);
      realp(ind)=imagp(ind)=0.0;
    }
    if (highs && (orderg % 2)) {
      for (i=orderg/2-(highs); i<=orderg/2+(highs); i++)
        realp(i)=imagp(i)=0.0;
    }
    else if (highs) {
      for (i=orderg/2-(highs-1); i<=orderg/2+(highs-1); i++)
        realp(i)=imagp(i)=0.0;
    }
    VB_Vector::complexIFFTReal(realp,imagp,exofilt);
  }

  // mean center it
  total=0.0;
  for (i=0; i<orderg; i++)
    total+=exofilt(i);
  total /= orderg;
  for (i=0; i<orderg; i++)
    exofilt(i)=exofilt(i)-total;

  // remove any phase component
  VB_Vector PS(exofilt.getLength());
  exofilt.getPS(PS);
  for (i=0; i<(int)PS.getLength(); i++)
    PS(i)=sqrt(PS(i));
  VB_Vector imagp(PS.getLength());
  PS.ifft(exofilt,imagp);

  // norm mag it
  exofilt.normMag();

  // write it out
  Vec tmpv(exofilt.getLength());
  MCopy(tmpv,exofilt);

  stringstream tmps;
  tmps.str("");
  tmps << "low frequencies filtered out: " << lows;
  tmpv.AddHeader(tmps.str());
  tmps.str("");
  tmps << "high frequencies filtered out: " << highs;
  tmpv.AddHeader(tmps.str());
  tmps.str("");
  tmps << "convolution kernel: " << kernelname;
  tmpv.AddHeader(tmps.str());

  tmpv.WriteFile(exofiltname.c_str());
  printf("[I] vbmakefilter: wrote filter %s\n",exofiltname.c_str());
  return 0;   // no error
}

// utility format conversion functions

void
MCopy(VB_Vector &c,Vec &vec)
{
  for (int i=0; i<vec.size(); i++)
    c(i)=vec[i];
}

void
MCopy(Vec &vec,VB_Vector &c)
{
  for (int i=0; i<vec.size(); i++)
    vec[i]=c(i);
}

void
vbmakefilter_help()
{
  printf("\nVoxBo vbmakefilter (v%s)\n",vbversion.c_str());
  printf("usage: vbmakefilter [flags]\n");
  printf("flags:\n");
  printf("    -e <fname>          create exofilt\n");
  printf("    -i <fname>          create intrinsic noise model\n");
  printf("    -t <TRs> <msecs>    how many TRs\n");
  printf("    -lf <lows>          low frequencies to remove\n");
  printf("    -mf <middles>       other frequencies to remove\n");
  printf("    -hf <highs>         high frequencies to remove\n");
  printf("    -k <kernel>         convolution kernel for exofilt\n");
  printf("    -n <noisemodel>     1/f noise model file name\n");
  printf("    -h                  help\n");
  printf("    -v                  version\n");
}


void
vbmakefilter_version()
{
  printf("VoxBo vb2tes (v%s)\n",vbversion.c_str());
}
