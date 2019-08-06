
// trialavg.cpp
// parse spec file for trial averaging
// Copyright (c) 2006 by The VoxBo Development Team

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

#include <fstream>
#include "glmutil.h"
#include "vbio.h"
#include "vbutil.h"

// sample syntax

// average <name>
//   units [time/ms/vols/volumes/vol]
//   interval <ms>
//   nsamples <n>
//   tr <ms>
//   trial XX [can be multiple floats]
//   trialset first interval count
// end

void TASpec::init() {
  startpositions.clear();
  interval = 1.0;
  nsamples = 10;
  TR = 3000.0;
  units = ta_vols;
  name = "trialaverage";
}

int TASpec::parsefile(string fname) {
  const int BUFLEN = 1024;
  ifstream fs;
  char buf[BUFLEN];

  fs.open(fname.c_str(), ios::in);
  if (!fs) return 100;
  while (fs.getline(buf, BUFLEN, '\n'))
    if (parseline(buf)) return 102;
  fs.close();
  return 0;
}

int TASpec::parseline(string line) {
  tokenlist args, sublist;
  args.ParseLine(line);
  sublist.SetSeparator(" \t,/");
  string cmd = vb_tolower(args[0]);

  if (args.size() == 0) return 0;
  if (args[0][0] == '#') return 0;
  if (args[0] == "units" && args.size() == 2) {
    if (args[1] == "time" || args[1] == "s")
      units = ta_time;
    else if (args[1] == "vols" || args[1] == "volumes" || args[1] == "vol")
      units = ta_vols;
    else
      return 101;
    return 0;
  } else if (args[0] == "interval" && args.size() == 2) {
    interval = strtod(args[1]);
    return 0;
  } else if (args[0] == "nsamples" && args.size() == 2) {
    nsamples = strtol(args[1]);
    return 0;
  } else if (args[0] == "trial" || args[0] == "trials") {
    for (size_t i = 1; i < args.size(); i++) {
      sublist.ParseLine(args[i]);  // reparse for things like 2,3,4
      for (size_t j = 0; j < sublist.size(); j++) {
        double val = strtod(sublist[j]);
        if (units == ta_time) val /= TR;
        startpositions.push_back(val);
      }
    }
    return 0;
  } else if (args[0] == "trialset" && args.size() == 4) {
    addtrialset(strtod(args[1]), strtod(args[2]), strtol(args[3]));
    return 0;
  } else if (args[0] == "tr") {
    TR = strtod(args[1]);
    return 0;
  } else
    return 102;
}

void TASpec::addtrialset(double first, double trialinterval, int count) {
  double pos = first;
  if (units == ta_time) {
    pos /= TR;
    trialinterval /= TR;
  }

  for (int i = 0; i < count; i++) {
    startpositions.push_back(pos);
    pos += trialinterval;
  }
}

VB_Vector TASpec::getTrialAverage(VB_Vector &data) {
  double volinterval;
  if (units == ta_vols)
    volinterval = interval;
  else
    volinterval = interval / TR;

  VB_Vector ta(nsamples);
  ta *= 0.0;
  VB_Vector xa(data.size());
  // for convenience
  double *xptr = xa.getTheVector()->data;
  double *yptr = data.getTheVector()->data;
  gsl_interp *myinterp;
  //   if (linearflag)
  //     myinterp=gsl_interp_alloc(gsl_interp_linear,points);
  //   else
  myinterp = gsl_interp_alloc(gsl_interp_cspline, data.size());
  for (size_t i = 0; i < data.size(); i++) xa[i] = i;
  gsl_interp_init(myinterp, xptr, yptr, data.size());
  double pos, val;
  for (int i = 0; i < (int)startpositions.size(); i++) {
    for (int j = 0; j < nsamples; j++) {
      pos = startpositions[i] + volinterval * (double)j;
      val = gsl_interp_eval(myinterp, xptr, yptr, pos, NULL);
      ta[j] += val;
    }
  }
  // divide by trial count
  for (int j = 0; j < nsamples; j++) ta[j] /= (double)startpositions.size();
  return ta;
}

void TASpec::print() {
  printf("trialaverage spec %s\n", name.c_str());
  printf("   count: %d\n", (int)startpositions.size());
  printf(" samples: %d\n", nsamples);
  printf("      TR: %.2f\n", TR);
  printf("interval: %.3f %s\n\n", interval,
         (units == ta_time ? "secs" : "vols"));
}

vector<TASpec> parseTAFile(string fname) {
  const int BUFLEN = 1024;
  ifstream fs;
  char buf[BUFLEN];
  tokenlist args;
  vector<TASpec> talist;
  TASpec myspec;

  fs.open(fname.c_str(), ios::in);
  if (!fs) return talist;
  bool inside = 0;
  while (fs.getline(buf, BUFLEN, '\n')) {
    args.ParseLine(buf);
    if (args.size() == 0) continue;
    if (args[0][0] == '#') continue;
    string cmd = vb_tolower(args[0]);
    if (!inside && cmd != "average") {
      fs.close();
      return talist;
    }
    if (!inside && args.size() != 2) {
      fs.close();
      return talist;
    }
    if (!inside) {
      myspec.init();
      myspec.name = args[1];
      inside = 1;
      continue;
    }
    // we're inside, either pass or end
    if (cmd == "end") {
      talist.push_back(myspec);
      inside = 0;
    } else if (myspec.parseline(buf)) {
      fs.close();
      return talist;
    }
  }
  fs.close();
  return talist;
}
