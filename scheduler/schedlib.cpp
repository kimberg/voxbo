
// schedlib.cpp
// VoxBo job scheduling util functions
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

using namespace std;

#include <sys/signal.h>
#include <sys/un.h>
#include <list>
#include "vbutil.h"
#include "vbprefs.h"
#include "vbjobspec.h"
#include "voxbo.h"
#include "schedlib.h"

void
read_queue(string queuedir,map<int,VBSequence> &seqlist)
{
  seqlist.clear();
  int err;
  struct stat st;
  string seqpat=queuedir+"/[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]";
  vglob vg(seqpat);
  for(size_t i=0; i<vg.size(); i++) {
    string seqdir=vg[i];
    err=stat(seqdir.c_str(),&st);
    if (err || !(S_ISDIR(st.st_mode))) continue;
    err=stat((seqdir+"/info.seq").c_str(),&st);
    if (err) {
      // probably a sequence dir with orphaned log files
      rmdir_force(seqdir);
      continue;
    }
    VBSequence seq(seqdir);
    seqlist[seq.seqnum]=seq;
  }
}

int
should_refract(VBJobSpec &js)
{
  // if it's never started, don't refract!
  if (js.startedtime==0)
    return 0;
  // if the last start actually finished, then it's okay to try again
  if (js.finishedtime >= js.startedtime)
    return 0;
  // if it's been more than RESEND_WAIT secs, don't refract
  if ((time(NULL) - js.startedtime) > RESEND_WAIT)
    return 0;
  // run out of reasons to let it go, better hold it up
  return 1;
}

// int
// has_unsatisfied_dependencies(SI seq,JI js)
// {
//   int jn,satisfied;
  
//   for (int i=0; i<(int)js->waitfor.size(); i++) {
//     jn=js->waitfor[i];
//     // seq=js.sequence;
//     satisfied=0;
//     for (JI j=seq->speclist.begin(); j<seq->speclist.end(); j++) {
//       // if it's satisfied, break and move on to the next one.  if
//       // it's not, we can pass on this job immediately
//       if (j->jnum==jn) {
//         if (j->status=='D') {satisfied =1; break; }
//         else return 1;
//       }
//     }
//     if (!satisfied)
//       return 1;
//   }
//   return 0;
// }

int
has_unsatisfied_dependencies(JI js,char donetable[],int maxjnum)
{
  vbforeach(int32 jn,js->waitfor) {
    if (jn>maxjnum) continue;  // bound checking, ignore on failure
    if (!(donetable[jn]))
      return 1;
  }
  return 0;
}

void
remove_seqwait(string queuedir,int snum)
{
  vglob vg((format("%s/waitfor.%d")%queuedir%snum).str());
  for (size_t i=0; i<vg.size(); i++)
    unlink(vg[i].c_str());
}
