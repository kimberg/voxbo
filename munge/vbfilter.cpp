
// vbfilter.cpp
// does some simple frequency filtering of 4D data
// Copyright (c) 2008 by The VoxBo Development Team

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
#include "vbutil.h"
#include "vbio.h"
#include "vbfilter.hlp.h"

class VBFilter {
public:
  enum ftype {hp,lp,bp,bs,rl,rh,rm} type;
  double f1,f2;
  int nremove;
  vector<int> nlist;
  VBFilter(ftype mytype,double a) {type=mytype;f1=a;}
  VBFilter(ftype mytype,double a,double b) {type=mytype;f1=a;f2=b;}
  VBFilter(ftype mytype,uint32 loworhigh) {type=mytype;nremove=loworhigh;}
  VBFilter(ftype mytype,string mynlist) {type=mytype;nlist=numberlist(mynlist);}
};

void vbfilter_help();
void vbfilter_version();
void filter_tes(Tes &tes,vector<VBFilter>filterlist,double period);
VB_Vector make_filter(int len,double period,vector<VBFilter>filterlist);
void filter_signal(VB_Vector &signal,VB_Vector filtermask);
void print_filter(VB_Vector &fmask,double period);

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vbfilter_help();
    exit(0);
  }
  tokenlist args;
  string outfile,writefile;
  vector<string> filelist;
  vector<VBFilter> filterlist;
  double period=0;
  bool infoflag=0;

  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-lp" && i<args.size()-1) {
      filterlist.push_back(VBFilter(VBFilter::lp,strtod(args[i+1])));
      i++;
    }
    else if (args[i]=="-hp" && i<args.size()-1) {
      filterlist.push_back(VBFilter(VBFilter::hp,strtod(args[i+1])));
      i++;
    }
    else if (args[i]=="-bp" && i<args.size()-2) {
      filterlist.push_back(VBFilter(VBFilter::bp,strtod(args[i+1]),strtod(args[i+2])));
      i+=2;
    }
    else if (args[i]=="-bs" && i<args.size()-2) {
      filterlist.push_back(VBFilter(VBFilter::bs,strtod(args[i+1]),strtod(args[i+2])));
      i+=2;
    }
    else if (args[i]=="-rl" && i<args.size()-1) {
      filterlist.push_back(VBFilter(VBFilter::rl,(uint32)strtol(args[++i])));
    }
    else if (args[i]=="-rh" && i<args.size()-1) {
      filterlist.push_back(VBFilter(VBFilter::rh,(uint32)strtol(args[++i])));
    }
    else if (args[i]=="-rm" && i<args.size()-1) {
      filterlist.push_back(VBFilter(VBFilter::rm,args[++i]));
    }
    else if (args[i]=="-p" && i<args.size()-1)
      period=strtod(args[++i]);
    else if (args[i]=="-v") {
      vbfilter_version();
      exit(0);
    }
    else if (args[i]=="-i") {
      infoflag=1;
    }
    else if (args[i]=="-h") {
      vbfilter_help();
      exit(0);
    }
    else if (args[i]=="-o" && i<args.size()-1)
      outfile=args[++i];
    else {
      filelist.push_back(args[i]);
    }
  }

  for (size_t i=0; i<filelist.size(); i++) {
    if (outfile.size())
      writefile=outfile;
    else
      writefile=xdirname(filelist[i])+"/f"+xfilename(filelist[i]);
    Tes mytes;
    VB_Vector myvec;
    double myperiod=0;
    if (mytes.ReadHeader(filelist[i])==0) {
      myperiod=period;
      if (mytes.voxsize[3]>FLT_MIN)
        myperiod=mytes.voxsize[3];
      if (myperiod==0 && period==0) {
        printf("[E] vbfilter: no sampling rate information for file %s, skipping\n",filelist[i].c_str());
        continue;
      }
      if (infoflag) {
        printf("[I] vbfilter: 4D file %s\n",filelist[i].c_str());
        printf("[I] vbfilter: %d observations, sampling period of %f\n",
               mytes.dimt,myperiod);
        VB_Vector fmask=make_filter(mytes.dimt,myperiod,filterlist);
        print_filter(fmask,myperiod);
      }
      else {
        printf("[I] vbfilter: filtering 4D volume %s...",filelist[i].c_str()); fflush(stdout);
        filter_tes(mytes,filterlist,myperiod);
        mytes.WriteFile(writefile);
        printf("done.\n");
      }
    }
    else if (myvec.ReadFile(filelist[i])==0) {
      myperiod=period;   // for now, always use period from command line
      if (myperiod==0) {
        printf("[E] vbfilter: no sampling rate information for file %s, skipping\n",filelist[i].c_str());
        continue;
      }
      VB_Vector fmask=make_filter(myvec.size(),period,filterlist);
      if (infoflag) {
        printf("[I] vbfilter: vector file %s\n",filelist[i].c_str());
        printf("[I] vbfilter: %d observations, sampling period of %f\n",
               (int)(myvec.size()),myperiod);
        print_filter(fmask,period);
      }
      else {
        printf("[I] vbfilter: filtering vector %s...",filelist[i].c_str()); fflush(stdout);
        filter_signal(myvec,fmask);
        myvec.WriteFile(writefile);
        printf("done.\n");
      }
    }
    else {
      printf("[E] vbfilter: couldn't read %s as either a vector or a 4D file\n",filelist[i].c_str());
      continue;
    }
  }
  exit(0);
}

void
filter_tes(Tes &tes,vector<VBFilter>filterlist,double period)
{
  tes.ReadData(tes.GetFileName());
  VB_Vector fmask=make_filter(tes.dimt,period,filterlist);
  for (int i=0; i<tes.dimx; i++) {
    for (int j=0; j<tes.dimy; j++) {
      for (int k=0; k<tes.dimz; k++) {
        tes.GetTimeSeries(i,j,k);
        filter_signal(tes.timeseries,fmask);
        for (int m=0; m<tes.dimt; m++)
          tes.SetValue(i,j,k,m,tes.timeseries[m]);
      }
    }
  }
}

VB_Vector
make_filter(int len,double period,vector<VBFilter>filterlist)
{
  int nfreq=len/2;
  VB_Vector freqmask(len);
  freqmask*=0.0;
  freqmask+=1.0;
  period/=1000.0;  // need it in seconds, not ms
  for (size_t i=0; i<filterlist.size(); i++) {
    for (int j=1; j<=nfreq; j++) {
      double thisfreq=(double)j/((double)len*period);
      int filterme=0;
      if (0)
        ;
      else if (filterlist[i].type==VBFilter::hp) {
        if (thisfreq<filterlist[i].f1) 
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::lp) {
        if (thisfreq>filterlist[i].f1) 
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::bp) {
        if (thisfreq<filterlist[i].f1 || thisfreq>filterlist[i].f2) 
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::bs) {
        if (thisfreq>=filterlist[i].f1 && thisfreq<=filterlist[i].f2) 
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::rl) {
        if (j<=filterlist[i].nremove)
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::rh) {
        if (nfreq-j+1<=filterlist[i].nremove)
          filterme=1;
      }
      else if (filterlist[i].type==VBFilter::rm) {
        if (find(filterlist[i].nlist.begin(),filterlist[i].nlist.end(),j)!=filterlist[i].nlist.end())
          filterme=1;
      }
      if (filterme) {
        freqmask(j)=0;
        freqmask(len-j)=0;
      }
    }
  }
  return freqmask;
}

void
print_filter(VB_Vector &fmask,double period)
{
  period/=1000.0;  // need it in seconds, not ms
  for (uint32 i=1; i<(fmask.size()/2)+1; i++) {
    double thisfreq=(double)i/((double)fmask.size()*period);
    printf("  frequency %3.d: %f%s\n",i,thisfreq,
	   (fmask[i] ? "" : " to be removed"  ));
  }
}

void
filter_signal(VB_Vector &signal,VB_Vector freqmask)
{
  VB_Vector sig_real(signal.getLength());
  VB_Vector sig_imag(signal.getLength());
  signal.fft(sig_real,sig_imag);
  for (size_t i=0; i<signal.getLength(); i++) {
    if (freqmask[i]<0.5)
      sig_real(i)=sig_imag(i)=0.0;
  }
  VB_Vector::complexIFFTReal(sig_real,sig_imag,signal);
}

void
vbfilter_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbfilter_version()
{
  printf("VoxBo vbfilter (v%s)\n",vbversion.c_str());
}
