
// vboverlap.cpp
// mask/lesion summary utility, mostly for counting unique voxels
// Copyright (c) 2008-2010 by The VoxBo Development Team

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

#include <math.h>
#include <map>
#include "vbutil.h"
#include "vbio.h"
#include "vbversion.h"
#include "vboverlap.hlp.h"

void vboverlap_help();
void vboverlap_version();

void calc_overlap(Cube &atlas,Cube &mask,int maskval,int atlasval);

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vboverlap_help();
    exit(0);
  }

  tokenlist args,filelist;
  args.Transfer(argc-1,argv+1);
  string atlasname;
  int maskval=-1;
  int atlasval=-1;
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-h") {
      vboverlap_help();
      exit(0);
    }
    else if (args[i]=="-a" && i<args.size()-1) {
      atlasname=args[++i];
    }
    else if (args[i]=="-m" && i<args.size()-1) {
      maskval=strtol(args[++i]);
    }
    else if (args[i]=="-r" && i<args.size()-1) {
      atlasval=strtol(args[++i]);
    }
    else if (args[i]=="-v") {
      vboverlap_version();
      exit(0);
    }
    else
      filelist.Add(args[i]);
  }

  if (filelist.size()<1 || atlasname=="") {
    vboverlap_help();
    exit(5);
  }

  Cube atlas,mask;
  Tes tmask;
  if (atlas.ReadFile(atlasname)) {
    printf("[E] vboverlap: couldn't read atlas %s\n",atlasname.c_str());
    exit(20);
  }
  for (size_t i=0; i<filelist.size(); i++) {
    if (!mask.ReadFile(filelist[i])) {
      calc_overlap(atlas,mask,maskval,atlasval);
    }
    else if (!tmask.ReadFile(filelist[i])) {
      for (int i=0; i<tmask.dimt; i++) {
        tmask.getCube(i,mask);
        mask.filename=(format("%03d")%i).str();
        calc_overlap(atlas,mask,maskval,atlasval);
      }
    }
    else {
      printf("[E] vboverlap: couldn't read mask file %s\n",filelist(i));
      continue;
    }
  }
  exit(0);
}

class reginfo {
public:
  int total;
  int overlap;
  reginfo(){total=0; overlap=0;}
};

void
calc_overlap(Cube &atlas,Cube &mask,int maskval,int atlasval)
{
  if (atlas.dimx!=mask.dimx || atlas.dimy!=mask.dimy||atlas.dimz!=mask.dimz) {
    printf("[E] vboverlap: %s: doesn't match atlas dimensions\n",
	   mask.GetFileName().c_str());
    return;
  }
  map<int,reginfo> breakdown;
  typedef map<int,reginfo>::iterator MI;
  int masktotal=0,inmask,aval,mval;
  for (int i=0; i<atlas.dimx; i++) {
    for (int j=0; j<atlas.dimy; j++) {
      for (int k=0; k<atlas.dimz; k++) {
        aval=(int)atlas.GetValue(i,j,k);
        if (aval==0) continue;
        if (atlasval>-1 && atlasval!=aval) continue;
        mval=(int)mask.GetValue(i,j,k);
        if ((mval>0 && maskval<0) || (mval==maskval))
          inmask=1;
        else
          inmask=0;
        if (inmask) masktotal++;
        breakdown[aval].total++;
        if (inmask) breakdown[aval].overlap++;
      }
    }
  }
  printf("[I] vboverlap: atlas %s, mask %s:\n",atlas.GetFileName().c_str(),mask.GetFileName().c_str());
  printf("[I]   mask includes %d ",masktotal);
  if (maskval<0)
    printf("nonzero voxels\n");
  else
    printf("voxels with value %d\n",maskval);
  for (MI mm=breakdown.begin(); mm!=breakdown.end(); mm++) {
    //if (mm->second.overlap==0) continue;
    string rname;
    if (atlas.maskspecs.count(mm->first))
      rname=atlas.maskspecs[mm->first].name;
    else
      rname="anon";
    printf("[I]   mask %s, region %d (%s): mask includes %d of %d voxels (%.4f)\n",
           mask.GetFileName().c_str(),
           mm->first,rname.c_str(),mm->second.overlap,mm->second.total,
           (float)mm->second.overlap/(float)mm->second.total);
  }
}

void
vboverlap_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vboverlap_version()
{
  printf("VoxBo vboverlap (v%s)\n",vbversion.c_str());
}
