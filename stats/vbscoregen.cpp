
// vbscoregen.cpp
// simulate scores from a mask
// Copyright (c) 2011 by The VoxBo Development Team

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

#include <fstream>
#include "vbio.h"
#include "vbscoregen.hlp.h"
#include "vbutil.h"

void vbscoregen_help();
void vbscoregen_version();

class VBscoregen {
 public:
  VBscoregen(string cfg) { parseconfig(cfg); }

 private:
  Tes lesions;
  string lesionfile;
  // map<string,VBRegion> maskmap;
  // map<string,VBVoxel> voxelmap;
  map<string, VB_Vector> vectormap;
  double mean1, sd1;
  double mean2, sd2;
  double floor, ceiling;
  VB_Vector scores;
  gsl_rng *rng;
  uint32 seed;
  void parseconfig(string configfile);
  double genscore(double pct);
  void generate(string fname);
  void do_if(tokenlist &args);
  void do_ifor(tokenlist &args);
  void do_ifand(tokenlist &args);
  void do_pct(tokenlist &args);
  void do_pctmin(tokenlist &args);
  void do_pctmax(tokenlist &args);
  double gaussian_random(double sigma);
};

int main(int argc, char **argv) {
  if (argc != 2) {
    vbscoregen_help();
    exit(0);
  }
  string arg = argv[1];
  if (arg == "-v") {
    vbscoregen_version();
    exit(0);
  }
  VBscoregen sg(arg);
}

double VBscoregen::genscore(double pct) {
  double mean = mean1 + (pct * (mean2 - mean1));
  double sd = sd1 + (pct * (sd2 - sd1));
  double ret = mean + gaussian_random(sd);
  if (ret < floor) ret = floor;
  if (ret > ceiling) ret = ceiling;
  return ret;
}

void VBscoregen::parseconfig(string configfile) {
  // init stuff here!
  floor = -FLT_MAX;
  ceiling = FLT_MAX;
  seed = VBRandom();
  rng = NULL;

  ifstream infile;
  infile.open(configfile.c_str());
  if (!infile) {
    // FIXME
    exit(111);
  }
  // chdir(xdirname(configfile).c_str());

  char buf[STRINGLEN];
  tokenlist args;
  while (infile.getline(buf, STRINGLEN, '\n')) {
    args.ParseLine(buf);
    if (args.size() == 0) continue;
    if (args[0][0] == '#' || args[0][0] == ';') continue;
    if (args[0] == "intact" && args.size() == 3) {
      mean1 = strtod(args[1]);
      sd1 = strtod(args[2]);
    } else if (args[0] == "damaged" && args.size() == 3) {
      mean2 = strtod(args[1]);
      sd2 = strtod(args[2]);
    } else if (args[0] == "bounds" && args.size() == 3) {
      floor = strtod(args[1]);
      ceiling = strtod(args[2]);
    } else if (args[0] == "seed" && args.size() == 2)
      seed = strtol(args[1]);
    else if (args[0] == "mask" && args.size() == 3) {
      Cube m;
      if (m.ReadFile(args[2])) {
        cout << "[E] vbscoregen: couldn't read mask file " << args[2] << endl;
        exit(202);
      }
      VBRegion rr = findregion_mask(m, vb_agt, 0.0);
      assert(rr.size());
      // maskmap[args[1]]=rr;
      // cout loaded mask xxxx stats
      vector<string> ll;
      ll.push_back(lesionfile);
      vectormap[args[1]] = getRegionTS(ll, rr, 0);
    } else if (args[0] == "voxel" && args.size() == 5) {
      VBVoxel vv(strtol(args[2]), strtol(args[3]), strtol(args[4]));
      // voxelmap[args[1]]=vv;
      // cout loaded voxel xxxx location
      VBRegion rr;
      rr.add(vv);
      vector<string> ll;
      ll.push_back(lesionfile);
      vectormap[args[1]] = getRegionTS(ll, rr, 0);
    } else if (args[0] == "lesions" && args.size() == 2) {
      lesionfile = args[1];
      if (lesions.ReadHeader(args[1])) {
        cout << "[E] vbscoregen: unable to read " << args[1] << endl;
        exit(191);
      }
      scores.resize(lesions.dimt);
    } else if (args[0] == "if" && args.size() == 4) {
      do_if(args);
    } else if (args[0] == "ifor" && args.size() == 5) {
      do_ifor(args);
    } else if (args[0] == "ifand" && args.size() == 5) {
      do_ifand(args);
    }

    else if (args[0] == "pct" && args.size() == 3) {
      do_pct(args);
    } else if (args[0] == "pctmin" && args.size() == 4) {
      do_pctmin(args);
    } else if (args[0] == "pctmax" && args.size() == 4) {
      do_pctmax(args);
    } else {
      infile.close();
      cout << "[E] vbscoregen: unrecognized command " << args[0] << endl;
      exit(128);
    }
  }
  infile.close();
  return;
}

void VBscoregen::generate(string fname) {
  if (scores.size() == 0) {
    cout << "[E]             generate line encountered before lesions line\n";
    exit(172);
  }
  if (scores.WriteFile(fname)) {
    cout << "[E]             error writing score file " << fname << endl;
    exit(121);
  } else
    cout << "[I]             wrote score file " << fname << endl;
}

void VBscoregen::do_if(tokenlist &args) {
  double thresh = max(strtod(args[2]), DBL_MIN);
  string v1name = args[1];
  string refname = args[3];
  VB_Vector &v1 = vectormap[v1name];
  cout << format("[I] vbscoregen: IF %s > %g --> %s\n") % v1name % thresh %
              refname;
  assert(v1.size());
  for (int i = 0; i < lesions.dimt; i++) {
    if (fabs(v1[i]) > thresh)
      scores[i] = genscore(1.0);
    else
      scores[i] = genscore(0.0);
  }
  generate(refname);
}

void VBscoregen::do_ifor(tokenlist &args) {
  double thresh = max(strtod(args[3]), DBL_MIN);
  string v1name = args[1];
  string v2name = args[2];
  string refname = args[4];
  VB_Vector &v1 = vectormap[v1name];
  VB_Vector &v2 = vectormap[v2name];
  cout << format("[I] vbscoregen: IF %s OR %s > %g --> %s\n") % v1name %
              v2name % thresh % refname;
  assert(v1.size());
  assert(v2.size());
  for (int i = 0; i < lesions.dimt; i++) {
    if (fabs(v1[i]) > thresh || fabs(v2[i]) > thresh)
      scores[i] = genscore(1.0);
    else
      scores[i] = genscore(0.0);
  }
  generate(refname);
}

void VBscoregen::do_ifand(tokenlist &args) {
  double thresh = max(strtod(args[3]), DBL_MIN);
  string v1name = args[1];
  string v2name = args[2];
  string refname = args[4];
  VB_Vector &v1 = vectormap[v1name];
  VB_Vector &v2 = vectormap[v2name];
  cout << format("[I] vbscoregen: IF %s OR %s > %g --> %s\n") % v1name %
              v2name % thresh % refname;
  assert(v1.size());
  assert(v2.size());
  for (int i = 0; i < lesions.dimt; i++) {
    if (fabs(v1[i]) > thresh && fabs(v2[i]) > thresh)
      scores[i] = genscore(1.0);
    else
      scores[i] = genscore(0.0);
  }
  generate(refname);
}

void VBscoregen::do_pct(tokenlist &args) {
  string v1name = args[1];
  string refname = args[2];
  VB_Vector &v1 = vectormap[v1name];
  cout << format("[I] vbscoregen: PCT %s --> %s\n") % v1name % refname;
  assert(v1.size());
  for (int i = 0; i < lesions.dimt; i++) {
    scores[i] = genscore(v1[i]);
  }
  generate(refname);
}

void VBscoregen::do_pctmin(tokenlist &args) {
  string v1name = args[1];
  string v2name = args[2];
  string refname = args[3];
  VB_Vector &v1 = vectormap[v1name];
  VB_Vector &v2 = vectormap[v2name];
  cout << format("[I] vbscoregen: PCTMIN %s/%s --> %s\n") % v1name % v2name %
              refname;
  assert(v1.size());
  assert(v2.size());
  for (int i = 0; i < lesions.dimt; i++) {
    scores[i] = genscore(min(v1[i], v2[i]));
  }
  generate(refname);
}

void VBscoregen::do_pctmax(tokenlist &args) {
  string v1name = args[1];
  string v2name = args[2];
  string refname = args[3];
  VB_Vector &v1 = vectormap[v1name];
  VB_Vector &v2 = vectormap[v2name];
  cout << format("[I] vbscoregen: PCTMIN %s/%s --> %s\n") % v1name % v2name %
              refname;
  assert(v1.size());
  assert(v2.size());
  for (int i = 0; i < lesions.dimt; i++) {
    scores[i] = genscore(max(v1[i], v2[i]));
  }
  generate(refname);
}

double VBscoregen::gaussian_random(double sigma) {
  if (!rng) {
    rng = gsl_rng_alloc(gsl_rng_mt19937);
    assert(rng);
    gsl_rng_set(rng, seed);
  }
  return gsl_ran_gaussian(rng, sigma);
}

void vbscoregen_help() { cout << boost::format(myhelp) % vbversion; }

void vbscoregen_version() {
  printf("VoxBo vbscoregen (v%s)\n", vbversion.c_str());
}
