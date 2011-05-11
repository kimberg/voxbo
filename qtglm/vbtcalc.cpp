
// tcalc.cpp
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
// original version written by Tom King, modified severely by Dan
// Kimberg

#include <QApplication>
#include "threshcalc.h"
#include "statthreshold.h"
#include "vbtcalc.hlp.h"

int flag;  // FIXME sloppy way to not update when populating dialog

void vbtcalc_help();
void vbtcalc_version();
void tcalc_cmd(threshold &v);

int main( int argc, char ** argv )
{
  // set defaults
  threshold v;
  v.searchVolume=50000*3*3*3;
  v.numVoxels=50000;
  v.fwhm=2.0;
  v.effdf=15;
  v.denomdf=0;
  v.pValPeak=0.05;
  v.vsize[0]=3.0;
  v.vsize[1]=3.0;
  v.vsize[2]=3.0;
  v.pValExtent=0.05;
  v.clusterThreshold=0.001;
  // jic
  v.peakthreshold = 0;
  v.pvalpeak = 0;
  v.bonpeakthreshold = 0;
  v.bonpvalpeak = 0;
  v.clusterthreshold = 0;
  v.peakthreshold1 = 0;
  v.pvalpeak1 = 0;
  v.extentthreshold = 0;
  v.extentthreshold1 = 0;
  v.pvalextent = 0;
  v.pvalextent1 = 0;

  bool f_cmd=0;

  tokenlist args;
  args.Transfer(argc-1,argv+1);
  
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-h") {
      vbtcalc_help();
      exit(0);
    }
    else if (args[i]=="-v") {
      vbtcalc_version();
      exit(0);
    }
    else if (args[i]=="-n" && i<args.size()-1) {
      v.numVoxels=strtol(args[++i]);
      v.searchVolume=lround(v.vsize[0]*v.vsize[1]*v.vsize[2])*v.numVoxels;
      f_cmd=1;
    }
    else if (args[i]=="-s" && i<args.size()-1) {
      v.fwhm=strtod(args[++i]);
      f_cmd=1;
    }
    else if (args[i]=="-z" && i<args.size()-3) {
      v.vsize[0]=strtod(args[++i]);
      v.vsize[1]=strtod(args[++i]);
      v.vsize[2]=strtod(args[++i]);
      v.searchVolume=lround(v.vsize[0]*v.vsize[1]*v.vsize[2])*v.numVoxels;
      f_cmd=1;
    }
    else if (args[i]=="-tdf" && i<args.size()-1) {
      v.effdf=strtod(args[++i]);
      v.denomdf=0;
      f_cmd=1;
    }
    else if (args[i]=="-fdf" && i<args.size()-2) {
      v.effdf=strtod(args[++i]);
      v.denomdf=strtod(args[++i]);
      f_cmd=1;
    }
    else if (args[i]=="-a" && i<args.size()-1) {
      v.pValPeak=strtod(args[++i]);
      f_cmd=1;
    }
    else {
      cout << "[E] vbtcalc: unrecognized option " << args[0] << endl;
      exit(1);
    }
  }
  if (f_cmd) {
    tcalc_cmd(v);
    exit(0);
  }
  QApplication a(argc,argv);
  tcalc tc(v);
  a.setMainWidget(&tc);
  tc.show();
  a.connect( &a,SIGNAL(lastWindowClosed()), &a,SLOT(quit()) );
  return a.exec();
}

void
tcalc_cmd(threshold &v)
{
  stat_threshold(v);
  double bestt=v.peakthreshold;
  if (v.bonpeakthreshold<v.peakthreshold)
    bestt=v.bonpeakthreshold;
  if (!(finite(bestt))) {
    cout << "No threhold could be calculated for the following parameters\n";
  }
  else if (v.denomdf>FLT_MIN) {
    cout << format("Critical value for F(%g,%g): %g\n")%
      v.effdf%v.denomdf%bestt;
  }
  else {
    cout << format("Critical value for t(%g): %g\n")%
      v.effdf%bestt;
  }
  if (finite(v.peakthreshold))
    cout << format("        RFT threshold: %g\n")%v.peakthreshold;
  else
    cout << format("        RFT threshold: none\n");
  if (finite(v.bonpeakthreshold))
    cout << format(" Bonferroni threshold: %g\n")%v.bonpeakthreshold;
  else
    cout << format(" Bonferroni threshold: none\n");
  cout << format("        search volume: %d mm^3\n")%v.searchVolume;
  cout << format("         total voxels: %d\n")%v.numVoxels;
  cout << format("FWHM smoothness in mm: %g\n")%v.fwhm;
  cout << format("                alpha: %g\n")%v.pValPeak;
  cout << format("          voxel sizes: %g %g %g\n")%v.vsize[0]%v.vsize[1]%v.vsize[2];
}

void
vbtcalc_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbtcalc_version()
{
  printf("VoxBo vbtcalc (v%s)\n",vbversion.c_str());
}
