
// vbhost.cpp
// voxbo host-handling code
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

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "vbutil.h"
#include "vbprefs.h"
#include "sys/time.h"
#include "vbjobspec.h"

VBHost::VBHost()
{
  init();
}

void
VBHost::init()
{
  resources.clear();
  reservations.clear();
  runninglist.clear();

  loadaverage=9999;
  memset(ncpus,0,168*sizeof(int));
  memset(pri,0,168*sizeof(int));
  currentpri=0;
  currentcpus=0;
  updatepid=0;
  lastresponse=0;
  hostname="";
  nickname="";
  memset(&addr,0,sizeof(struct sockaddr_in));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(0); // formerly serverport set here
  speed=0;
  rank=0;
  total_cpus=1;
  taken_cpus=0;
  avail_cpus=0;
  socket=-1;
  status="unknown";
  valid=0;
}

VBHost::VBHost(const string nn,const string hn,uint16 serverport)
{
  init();
  setnames(nn,hn);
  initaddress(serverport);
}

int
VBHost::ReadFile(const string &fname)
{
  init();
  char line[STRINGLEN];
  int i,j,day1,day2,hour1,hour2,mypri,mycpus,day,hour;
  tokenlist args;
  FILE *infile;

  // default name
  nickname=xfilename(fname);
  tokenlist items,item;
  items.SetSeparator(",");
  
  infile=fopen(fname.c_str(),"r");
  if (!infile) {
    nickname="";
    return 101;
  }
  
  for (i=0; i<168; i++)
    pri[i]=ncpus[i]=0;
  while(fgets(line,STRINGLEN,infile) != NULL) {
    if (args.ParseLine(line) < 1)
      continue;
    if (args[0]=="name" || args[0]=="nickname" || args[0]=="shortname")
      nickname=args[1];
    if (args[0]=="hostname")           // full hostname
      hostname=args[1];
    else if (args[0]=="speed") {       // speed (arbitrary value)
      speed = strtol(args[1]);
    }
    else if (args[0]=="rank") {        // rank
      rank = strtol(args[1]);
    }
    else if (args[0]=="resource" || args[0]=="globalresource" || args[0]=="provides") {
      if (args.size() == 3) {
        VBResource vbr;
        vbr.name=args[1];
        if (isdigit(args[2][0])) {
          vbr.cnt=strtol(args[2]);
          vbr.command="";
        }
        else {
          vbr.cnt=0;
          vbr.command=args[2];
        }
        if (args[0]=="globalresource")
          vbr.f_global=1;
        else
          vbr.f_global=0;
        resources[vbr.name]=vbr;
      }
      else if (args.size()==2) {
        VBResource vbr;
        vbr.name=args[1];
        vbr.cnt=0;
        resources[vbr.name]=vbr;
      }
    }
    else if (args[0]=="avail") {       // hours available
      if (args.size() < 5) {
        fprintf(stderr,"[E] invalid availability config line for server %s\n",nickname.c_str());
        continue;
      }
      sscanf(args[1].c_str(),"%d-%d",&day1,&day2);
      sscanf(args[2].c_str(),"%d-%d",&hour1,&hour2);
      mypri=strtol(args[3]);
      mycpus=strtol(args[4]);
      if (day2 < day1) day2 +=7;
      if (hour2 < hour1) hour2 += 24;
      for (i=day1; i<=day2; i++) {
        for (j=hour1; j<=hour2; j++) {
          day = i; hour = j;
          if (day > 6) day -= 7;
          if (hour > 23) hour -= 24;
          pri[(24*day)+hour]=mypri;
          ncpus[(24*day)+hour]=mycpus;
        }
      }
    }
    else if (args[0]=="endserver")
      break;
  }
  fclose(infile);

  return 0;  // no error!
}

void
VBHost::setnames(const string nn,const string hn)
{
  nickname=nn;
  hostname=hn;
}

void
VBHost::initaddress(uint16 serverport)
{
  memset(&addr,0,sizeof(struct sockaddr_in));
  addr.sin_family=AF_INET;
  addr.sin_port = htons(serverport);
  time_t tt=time(NULL);
  lastresponse=tt;
  // get host address
  struct hostent *hp;
  hp = gethostbyname(hostname.c_str());
  if (hp)
    if (memcpy(&addr.sin_addr,hp->h_addr_list[0],hp->h_length))
      valid=1;
}


string
VBHost::tobuffer(map<jobid,VBJobSpec> &runningmap)
{
  stringstream ss;
  ss << "[hostname "<<hostname<<"]";
  ss << "[nickname "+nickname<<"]";
  ss << "[currentpri "<< currentpri <<"]";
  ss << "[load "<<loadaverage<<"]";
  ss << "[total_cpus "<<total_cpus<<"]";
  ss << "[taken_cpus "<<taken_cpus<<"]";
  ss << "[avail_cpus "<<avail_cpus<<"]";
  ss << "[status "<<status<<"]";
  for (int j=0; j<(int)reservations.size(); j++) {
    ss<<"[reservation "<<reservations[j].owner<<" "<<reservations[j].start
      <<" "<<reservations[j].end<<" "<<reservations[j].reason<<"]";
  }
  for (RI rr=resources.begin();rr!=resources.end(); rr++)
    ss<<"[resource '"<<rr->second.name<<"' '"<<rr->second.f_global<<"' "<<
      rr->second.cnt<<"]";
  
  for (map<jobid,VBJobSpec>::iterator j=runningmap.begin(); j!=runningmap.end(); j++) {
    if (j->second.hostname!=nickname) continue;
    char info[STRINGLEN];
    sprintf(info,"[job %d %d %d %d %ld \"%s\"]",
            j->second.snum,j->second.jnum,j->second.actualcpus,j->second.percentdone,
            time(NULL)-j->second.startedtime,j->second.name.c_str());
    ss<<info;
  }
  return ss.str();
}

int
VBHost::frombuffer(string buf)
{
  tokenlist args,argx;
  args.SetQuoteChars("[<(\"'");
  argx.SetQuoteChars("[<(\"'");
  args.ParseLine(buf);
  for (size_t i=0; i<args.size(); i++) {
    argx.ParseLine(args[i]);
    if (argx[0]=="load")
      loadaverage=strtod(argx[1]);
    else if (argx[0]=="currentpri")
      currentpri=strtol(argx[1]);
    else if (argx[0]=="hostname")
      hostname=argx[1];
    else if (argx[0]=="nickname")
      nickname=argx[1];
    else if (argx[0]=="total_cpus")
      total_cpus=strtol(argx[1]);
    else if (argx[0]=="taken_cpus")
      taken_cpus=strtol(argx[1]);
    else if (argx[0]=="avail_cpus")
      avail_cpus=strtol(argx[1]);
    else if (argx[0]=="status")
      status=argx[1];
    else if (argx[0]=="reservation") {
      VBReservation rr;
      rr.owner=argx[1];
      rr.start=strtol(argx[2]);
      rr.end=strtol(argx[3]);
      rr.reason=argx[4];
      reservations.push_back(rr);
    }
    else if (argx[0]=="resource") {
      VBResource rr;
      rr.name=argx[1];
      if (strtol(args[2]))
        rr.f_global=1;
      else
        rr.f_global=0;
      rr.cnt=strtol(argx[3]);
      resources[rr.name]=rr;
    }
    else if (argx[0]=="job") {
      VBJobSpec jj;
      jj.snum=strtol(argx[1]);
      jj.jnum=strtol(argx[2]);
      jj.actualcpus=strtol(argx[3]);
      jj.percentdone=strtol(argx[4]);
      jj.startedtime=strtol(argx[5]);
      jj.name=argx[6];
      runninglist.push_back(jj);
    }
  }
  return 0;
}

float
VBHost::LoadAverage() const
{
  return loadaverage;
}

void
VBHost::SetLoadAverage(float load)
{
  loadaverage=load;
}

short
VBHost::Priority() const
{
  return currentpri;
}

void
VBHost::Update()
{
  // apparent cpus in use is calculated so that we need to be at 0.8
  // to count the cpu as taken
  int apparent = total_cpus - lround(loadaverage - 0.3);
  if (apparent < 0)
    apparent = 0;
  int bypolicy = total_cpus - taken_cpus;
  // conservatively, we'll go with the least favorable answer
  avail_cpus = (apparent < bypolicy ? apparent : bypolicy);
}

void
VBHost::DeadOrAlive()
{
}

void
VBHost::CheckSchedule()
{
  time_t now;
  struct tm *nowtm;
  int ind;
  
  now = time(NULL);
  nowtm = localtime(&now);
  ind = (nowtm->tm_wday * 24) + nowtm->tm_hour;
  currentpri = pri[ind];
  total_cpus = ncpus[ind];
  if (currentpri > 5) currentpri = 5;
  if (currentpri < 1) currentpri = 1;
}

// note that Ping() is now run asynchronously from a forked process.
// also, there are some bad consequences of a false alarm in declaring
// a machine missing.  so we have the timeouts set high (60s)

void
VBHost::Ping(map<jobid,VBJobSpec> &runningmap)
{
  if (!valid)      // host not responding
    return;

  int s,err;
  char info[STRINGLEN];
  tokenlist args,argx;

  // build list of snum/jnum/pids to check on
  string pinglist;
  time_t tt=time(NULL);
  for (map<jobid,VBJobSpec>::iterator j=runningmap.begin(); j!=runningmap.end(); j++) {
    // only ping our own jobs!
    if (j->second.hostname!=nickname) continue;
    if (tt-j->second.lastreport < 60) continue;
    pinglist+=(string)" "+strnum(j->second.snum)+" "+strnum(j->second.jnum)+" "+strnum(j->second.pid);
  }

  s=safe_connect(&addr,60.0);
  if (s < 0)
    return;

  // sends this small shouldn't block, so we're okay not to select
  string msg;
  msg="PHONEHOME";
  msg+=pinglist;
  err=send(s,msg.c_str(),msg.size(),0);
  if (err == -1) {
    close(s);
    return;
  }
  err=safe_recv(s,info,STRINGLEN,60.0);  // wait for ACK
  if (err<1) {
    close(s);
    return;
  }
  args.ParseLine(info);
  if (args[0]!="ACK")
    printf("[E] %s (%s) bad acknowledgment for phonehome: %s\n",
           timedate().c_str(),nickname.c_str(),args(0));
  close(s);
  return;
}

int
VBHost::SendMsg(string msg)
{
  if (!valid)      // host not responding
    return 101;

  int s,err;
  char info[STRINGLEN];
  tokenlist args;

  s=safe_connect(&addr,6.0);
  if (s < 0)
    return 102;

  // sends this small shouldn't block, so we're okay not to select
  err=send(s,msg.c_str(),msg.size(),0);
  if (err<0) {
    close(s);
    return 103;
  }
  err=safe_recv(s,info,STRINGLEN,10.0);  // wait for ACK
  if (err<1) {
    close(s);
    return 104;
  }
  args.ParseLine(info);
  if (args[0]!="ACK")
    printf("[E] bad acknowledgment for msg: %s\n",args(0));
  close(s);
  return 0;
}

bool
cmp_host_pri_taken(const VBHost &h1,const VBHost &h2)
{
  // a machine with a lower rank gets first crack
  if (h1.rank < h2.rank)
    return 1;
  if (h1.rank > h2.rank)
    return 0;
  // same rank, prioritize hosts that are less busy
  if (h1.taken_cpus < h2.taken_cpus)
    return 1;
  if (h1.taken_cpus > h2.taken_cpus)
    return 0;
  // beyond that it's random
  if (h1.rand < h2.rand)
    return 1;
  return 0;
}

bool
cmp_host_name(const VBHost &h1,const VBHost &h2)
{
  if (h1.nickname<h2.nickname)
    return 1;
  else
    return 0;
}

bool
cmp_host_random(const VBHost &,const VBHost &)
{
  if (0x01 & VBRandom())
    return 1;
  return 0;
}

void
VBHost::print()
{
  printf("HOST %s (load %f) (currentpri %d) (currentcpus %d)\n",
	 nickname.c_str(),loadaverage,currentpri,currentcpus);
  printf("    hostname: %s\n",hostname.c_str());
  printf("      status: %s\n",status.c_str());
  printf("  total_cpus: %d\n",total_cpus);
  printf("  taken_cpus: %d\n",taken_cpus);
  printf("  avail_cpus: %d\n",avail_cpus);
  printf("        rank: %d\n",rank);
  printf("     running: %d\n",(int)runninglist.size());
  printf("lastresponse: %ld seconds ago\n",time(NULL)-lastresponse);
  if (checkdirs.size()) {
    vbforeach(string dd,checkdirs) {
      printf("    checkdir: %s\n",dd.c_str());
    }
  }
  for (RI rr=resources.begin(); rr!=resources.end(); rr++)
    printf("  + resource %s %d\n",rr->second.name.c_str(),
           rr->second.cnt);
}

void
VBHost::updateresources()
{
  // FIXME
//   typedef pair<const string,VBResource> srtype;
//   vbforeach (srtype &rr,resources)
//     rr.second.current=rr.second.cnt;
//   vbforeach (VBJobSpec &js,runninglist) {
//     string jt=js.jobtype;
//     vbforeach(VBResource &jr,vbp.jobtypemap[jt].requires) {
//       VBResource *hr=&(resources[jr.name]);
//       if (hr->cnt)
//         hr->current-=jr.cnt;
//     }
//   }
}

VBResource::VBResource()
{
  name="";
  command="";
  f_global=0;
  cnt=0;
  current=0;
}

