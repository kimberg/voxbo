
// statthreshold.h
// library code header to produce thresholds
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
// by Tom King and Dan Kimberg, based on ideas by Keith Worsley

#ifndef STATTHRESHOLD_H
#define STATTHRESHOLD_H

#include <map>
#include <fstream>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_cdf.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_sf_gamma.h>
#include "vbio.h"
#include "gsl/gsl_complex.h"
#include "gsl/gsl_complex_math.h"

class threshold {
public:
  //input
  // searchvolume=numvoxels*vsizes -- stat_threshold only uses
  // searchvolume and numvoxels, so the former should be calculated
  // when appropriate
  long searchVolume;
  long numVoxels;
  double vsize[3];
  double fwhm;
  double effdf,denomdf;
  double pValPeak;
  double clusterThreshold;
  double pValExtent;
  //output
  double peakthreshold;
  double pvalpeak;
  double bonpeakthreshold;
  double bonpvalpeak;
  double clusterthreshold;
  double peakthreshold1;
  double pvalpeak1;
  double extentthreshold;
  double extentthreshold1;
  double pvalextent;
  double pvalextent1;
};

int stat_threshold(threshold &v);

#endif // STATTHRESHOLD_H
