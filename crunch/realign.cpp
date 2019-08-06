
// realign.cpp
// VoxBo realignment (motion correction) module
// Copyright (c) 1998-2003 by The VoxBo Development Team

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

// The guts of the underlying code were based on SPM source code
// written in MATLAB by John Ashburner.  See the SPM web site at:
// (http://www.fil.ion.bpmf.ac.uk/spm/)

using namespace std;

#include "vbcrunch.h"

const double TOLERANCE = 0.5;
const int MAXITERATIONS = 4;
const double SPIKETHRESH = 0.5;
const double DRIFTTHRESH = 0.5;

class Realignment {
 private:
  string refimage;
  string outfile;
  string paramfile;
  tokenlist args;
  CrunchCube *refcub, *sref, *mycub, *newcub;
  int startcub, i, o1, o2, o3;
  int maxiterations;
  int refnum;  // index of reference image within tes
  double spikethresh;
  double driftthresh;
  double tolerance;
  Matrix moveparams;
  double minvoxsize;
  int maxdim;
  int paramcheckonly;
  int summarizeflag;
  double tol_trans, tol_rot;
  Tes *mytes;                  // current tes file
  char timestring[STRINGLEN];  // text representation of time of realignment
 public:
  Realignment(int argc, char **argv);
  ~Realignment();
  int Crunch();
  void Check();
  void Summarize();
  void WriteMoveParams(const string &tesname, const string &outfile);
  void ParamFileCheck();
};

void realign_help();

int main(int argc, char *argv[]) {
  stringstream tmps;
  tzset();         // make sure all times are timezone corrected
  if (argc < 2) {  // not enough args, display autodocs
    realign_help();
    exit(0);
  }

  Realignment *r = new Realignment(argc, argv);  // init realign object
  if (!r) {
    tmps << "realign: couldn't allocate a tiny realignment structure";
    printErrorMsg(VB_ERROR, tmps.str());
    exit(5);
  }
  int error = r->Crunch();  // do the crunching
  delete r;                 // clean up
  exit(error);
}

Realignment::Realignment(int argc, char *argv[]) {
  refimage = "";
  refnum = 0;
  maxiterations = MAXITERATIONS;
  spikethresh = SPIKETHRESH;
  driftthresh = DRIFTTHRESH;
  tolerance = TOLERANCE;
  paramcheckonly = FALSE;
  summarizeflag = FALSE;

  for (i = 1; i < argc; i++) {
    if (dancmp(argv[i], "-r")) {
      if (i < argc - 1) {
        refimage = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-c")) {
      paramcheckonly = TRUE;
    } else if (dancmp(argv[i], "-q")) {
      paramcheckonly = TRUE;
      summarizeflag = TRUE;
    } else if (dancmp(argv[i], "-p")) {
      if (i < argc - 1) {
        paramfile = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-n")) {
      if (i < argc - 1) {
        refnum = strtol(argv[i + 1]);
        i++;
      }
    } else if (dancmp(argv[i], "-t")) {
      if (i < argc - 1) {
        tolerance = strtod(argv[i + 1], NULL);
        // should use some default if invalid
        i++;
      }
    } else if (dancmp(argv[i], "-i")) {
      if (i < argc - 1) {
        maxiterations = strtol(argv[i + 1], NULL, 10);
        i++;
      }
    } else if (dancmp(argv[i], "-s")) {
      if (i < argc - 1) {
        spikethresh = strtod(argv[i + 1], NULL);
        i++;
      }
    } else if (dancmp(argv[i], "-o")) {
      if (i < argc - 1) {
        outfile = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-d")) {
      if (i < argc - 1) {
        driftthresh = strtod(argv[i + 1], NULL);
        i++;
      }
    } else if (argv[i][0] != '-') {
      args.Add(argv[i]);
      continue;
    }
  }
}

Realignment::~Realignment() {}

void Realignment::ParamFileCheck() {
  int images;
  stringstream tmps;

  for (int i = 0; i < args.size(); i++) {
    VB_Vector mp(args[i]);
    if (!mp.size()) {
      tmps.str("");
      tmps << "realign: couldn't read parameters from file " << args[i];
      printErrorMsg(VB_ERROR, tmps.str());
      continue;
    }
    startcub = 0;
    maxdim = 64;
    minvoxsize = 3.75;
    images = mp.size() / 7;
    moveparams.resize(images, 7);
    for (int j = 0; j < mp.size(); j++) moveparams(j / 7, j % 7) = mp[j];
    tmps.str("");
    tmps << "realign: checking parameter file " << args[i];
    printErrorMsg(VB_INFO, tmps.str());
    if (summarizeflag)
      Summarize();
    else
      Check();
  }
}

int Realignment::Crunch() {
  if (paramcheckonly) {
    ParamFileCheck();
    return (0);
  }

  int i, iter, corrected;
  char tmp[STRINGLEN];
  struct tm *mytm;
  time_t mytime;
  stringstream tmps;

  for (i = 0; i < args.size(); i++) {
    startcub = 0;  // start with the first cube by default
    mytes = new Tes;
    mytes->ReadFile(args[i]);
    if (!mytes->data_valid) {
      tmps.str("");
      tmps << "realign: " << args[i] << " is not a valid 4D file.";
      printErrorMsg(VB_ERROR, tmps.str());
      delete mytes;
      return 105;  // panic and exit
    }
    // set default outfile name
    if (!outfile.size())
      outfile = xdirname(args[i]) + (string) "/r" + xfilename(args[i]);
    // guess default origin and load reference image
    if (i == 0 || (o1 == 0 && o2 == 0 && o3 == 0)) {
      minvoxsize = mytes->voxsize[0];
      if (mytes->voxsize[1] < minvoxsize) minvoxsize = mytes->voxsize[1];
      if (mytes->voxsize[2] < minvoxsize) minvoxsize = mytes->voxsize[2];
      if (minvoxsize < 0.0001) {
        tmps.str("");
        tmps << "realign: invalid voxel sizes for tes file " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
        delete mytes;
        return 150;
      }
      maxdim = mytes->dimx;
      if (mytes->dimy > maxdim) maxdim = mytes->dimy;
      if (mytes->dimz > maxdim) maxdim = mytes->dimz;

      tol_trans = minvoxsize * tolerance;
      tol_rot = atan(tolerance / ((float)maxdim / 2.0));

      if (mytes->origin[0] == 0 && mytes->origin[1] == 0 &&
          mytes->origin[2] == 0) {
        o1 = mytes->dimx;
        o2 = mytes->dimy;
        o3 = mytes->dimz;
        guessorigin(o1, o2, o3);
        mytes->SetOrigin(o1, o2, o3);
      } else {
        o1 = mytes->origin[0];
        o2 = mytes->origin[1];
        o3 = mytes->origin[2];
      }
      if (refimage.size()) {
        tmps.str("");
        tmps << "realign: loading reference image " << refimage;
        printErrorMsg(VB_INFO, tmps.str());
        refcub = new CrunchCube();
        // refcub->ReadHeader(refimage);
        refcub->ReadFile(refimage);
      } else {  // if no template specified, use first image
        if (refnum >= mytes->dimt) {
          tmps.str("");
          tmps << "realign: index " << refnum << "is large, using "
               << mytes->dimt - 1 << " instead";
          printErrorMsg(VB_INFO, tmps.str());
          refnum = 0;
        }
        if (refnum < 0) {
          tmps.str("");
          tmps << "realign: index " << refnum
               << " is less than zero, using the first image instead";
          printErrorMsg(VB_ERROR, tmps.str());
          refnum = 0;
        }
        tmps.str("");
        tmps << "realign: using image " << refnum << " as the reference image";
        printErrorMsg(VB_INFO, tmps.str());
        refcub = new CrunchCube((*mytes)[refnum]);
        startcub = 1;
      }
    }
    if (!refcub->data_valid) {
      tmps.str("");
      tmps << "realign: invalid reference volume";
      printErrorMsg(VB_ERROR, tmps.str());
      delete mytes;
      return 120;
    }
    if (refcub->dimx != mytes->dimx || refcub->dimy != mytes->dimy ||
        refcub->dimz != mytes->dimz ||
        refcub->voxsize[0] != mytes->voxsize[0] ||
        refcub->voxsize[1] != mytes->voxsize[1] ||
        refcub->voxsize[2] != mytes->voxsize[2]) {
      tmps.str("");
      tmps << "realign: tes file " << args[i]
           << " doesn't match the size/dimensions of the reference volume";
      printErrorMsg(VB_ERROR, tmps.str());
      delete mytes;
      return 130;
    }

    refcub->SetOrigin(o1, o2, o3);

    tmps.str("");
    tmps << "realign: realigning " << args[i];
    printErrorMsg(VB_INFO, tmps.str());

    moveparams.resize(mytes->dimt, 7);  // make room for movement parameters
    moveparams.fill(0.0);
    sref = dan_smooth_image(refcub, 8, 8, 8);
    sref->SetOrigin(o1, o2, o3);
    int percentinterval = mytes->dimt / 10;
    int countdown = percentinterval;
    for (int j = startcub; j < mytes->dimt; j++) {
      iter = 0;
      corrected = FALSE;
      while (iter < maxiterations && corrected == FALSE) {
        CrunchCube mycub;
        mytes->getCube(j, mycub);
        newcub = realign_twoimages(refcub, sref, &mycub);
        mytes->SetCube(j, newcub);
        if (newcub->transform(0) < tol_trans &&  // transform = movement params
            newcub->transform(1) < tol_trans &&
            newcub->transform(2) < tol_trans &&
            newcub->transform(3) < tol_rot && newcub->transform(4) < tol_rot &&
            newcub->transform(5) < tol_rot)
          corrected = TRUE;
        if (iter == 0) {  // add the movement params to the list
          moveparams(j, 0) = newcub->transform(0);
          moveparams(j, 1) = newcub->transform(1);
          moveparams(j, 2) = newcub->transform(2);
          moveparams(j, 3) = newcub->transform(3) * (180.0) / PI;
          moveparams(j, 4) = newcub->transform(4) * (180.0) / PI;
          moveparams(j, 5) = newcub->transform(5) * (180.0) / PI;
        }
        iter++;
        moveparams(j, 6) = iter;
        delete newcub;
      }
      countdown--;
      if (countdown == 0) {
        printf("Percent done: %d\n", (j * 100) / mytes->dimt);
        fflush(stdout);
        countdown = percentinterval;
      }
    }
    delete sref;
    mytime = time(NULL);
    mytm = localtime(&mytime);
    strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
    sprintf(tmp, "realign_date: %s", timestring);
    mytes->AddHeader(tmp);
    sprintf(tmp, "realign_tolerance: %.4f %.4f", tol_trans, tol_rot);
    mytes->AddHeader(tmp);
    if (refimage.size())
      sprintf(tmp, "realign_ref: %s", refimage.c_str());
    else
      sprintf(tmp, "realign_ref: [image:%d]", refnum);
    mytes->AddHeader(tmp);

    mytes->SetFileName(outfile);
    int err = mytes->WriteFile();

    if (!err) {
      tmps.str("");
      tmps << "realign: wrote " << outfile;
      printErrorMsg(VB_INFO, tmps.str());
      outfile.replace(outfile.size() - 4, 4, "_MoveParams.ref");
      WriteMoveParams(args[i], outfile);
      Check();
    } else {
      tmps.str("");
      tmps << "realign: error writing " << outfile;
      printErrorMsg(VB_ERROR, tmps.str());
      delete mytes;
      delete refcub;
      return 110;
    }
    delete mytes;
  }
  delete refcub;
  return (0);  // no error
}

void Realignment::WriteMoveParams(const string &tesname,
                                  const string &outfile) {
  int i;
  FILE *fp;

  if ((fp = fopen(outfile.c_str(), "w"))) {
    fprintf(fp, ";VB98\n;REF1\n");
    fprintf(fp, "; Movement params from motion correction of %s\n",
            tesname.c_str());
    if (refimage.size())
      fprintf(fp, "; aligned to cube %s\n", refimage.c_str());
    else
      fprintf(fp, "; aligned to the first image of series\n");
    fprintf(fp,
            "; The seven parameter values (x,y,z,pitch,roll,yaw,iterations)\n");
    fprintf(
        fp,
        "; are written in order for each time point, translations in mm,\n");
    fprintf(fp, "; rotations in degrees\n\n");
    for (i = 0; i < mytes->dimt; i++) {
      fprintf(fp, "%.6g\n", (double)moveparams(i, 0));
      fprintf(fp, "%.6g\n", (double)moveparams(i, 1));
      fprintf(fp, "%.6g\n", (double)moveparams(i, 2));
      fprintf(fp, "%.6g\n", (double)moveparams(i, 3));
      fprintf(fp, "%.6g\n", (double)moveparams(i, 4));
      fprintf(fp, "%.6g\n", (double)moveparams(i, 5));
      fprintf(fp, "%.6f\n", (double)moveparams(i, 6));
    }
    fclose(fp);
  }
}

void Realignment::Summarize() {
  double totaltrans = 0.0;
  double totalrot = 0.0;
  int i;
  int multiples = 0;

  for (i = startcub + 1; i < moveparams.rows(); i++) {
    totaltrans += abs(moveparams(i, 0));
    totaltrans += abs(moveparams(i, 1));
    totaltrans += abs(moveparams(i, 2));
    totalrot += abs(moveparams(i, 3));
    totalrot += abs(moveparams(i, 4));
    totalrot += abs(moveparams(i, 5));
    if (moveparams(i, 6) > 1) multiples++;
  }

  double meantrans = totaltrans / (double)(moveparams.rows() * 3);
  double meanrot = totalrot / (double)(moveparams.rows() * 3);
  printf("realign: mean translation: %.2f\n", meantrans);
  printf("realign: mean rotation: %.2f\n", meanrot);
  printf("realign: mulitply corrected images: %d\n", multiples);
}

void Realignment::Check() {
  double spike_mm = minvoxsize * spikethresh;
  double spike_degrees = atan(spikethresh / (maxdim / 2.0)) * (180.0 / PI);
  double drift_mm = minvoxsize * driftthresh;
  double drift_degrees = atan(driftthresh / (maxdim / 2.0)) * (180.0 / PI);
  int i, spikes[6], drifts[6];
  int multiples = 0;

  for (i = 0; i < 6; i++) {
    spikes[i] = drifts[i] = 0;
  }

  for (i = startcub + 1; i < moveparams.rows(); i++) {
    if ((moveparams(i, 0) - moveparams(i - 1, 0)) > spike_mm) spikes[0]++;
    if ((moveparams(i, 1) - moveparams(i - 1, 1)) > spike_mm) spikes[1]++;
    if ((moveparams(i, 2) - moveparams(i - 1, 2)) > spike_mm) spikes[2]++;
    if ((moveparams(i, 3) - moveparams(i - 1, 3)) > spike_degrees) spikes[3]++;
    if ((moveparams(i, 4) - moveparams(i - 1, 4)) > spike_degrees) spikes[4]++;
    if ((moveparams(i, 5) - moveparams(i - 1, 5)) > spike_degrees) spikes[5]++;
  }
  for (i = startcub; i < moveparams.rows(); i++) {
    if (moveparams(i, 0) > drift_mm) drifts[0]++;
    if (moveparams(i, 1) > drift_mm) drifts[1]++;
    if (moveparams(i, 2) > drift_mm) drifts[2]++;
    if (moveparams(i, 3) > drift_degrees) drifts[3]++;
    if (moveparams(i, 4) > drift_degrees) drifts[4]++;
    if (moveparams(i, 5) > drift_degrees) drifts[5]++;
    if (moveparams(i, 6) > 1.0) multiples++;
  }

  string filename;
  if (mytes)
    filename = mytes->GetFileName();
  else
    filename = "your parameter file";
  printf("Summary of extreme corrections for %s\n", filename.c_str());
  printf(
      "Thresholds: spike mm=%.2f drift mm=%.2f spike degrees=%.2f drift "
      "degrees=%.2f\n",
      spike_mm, drift_mm, spike_degrees, drift_degrees);
  printf("  Spikes in X dimension: %d\n", spikes[0]);
  printf("  Spikes in Y dimension: %d\n", spikes[1]);
  printf("  Spikes in Z dimension: %d\n", spikes[2]);
  printf("  Spikes in pitch: %d\n", spikes[3]);
  printf("  Spikes in roll: %d\n", spikes[4]);
  printf("  Spikes in yaw: %d\n", spikes[5]);

  printf("  Drifts in X dimension: %d\n", drifts[0]);
  printf("  Drifts in Y dimension: %d\n", drifts[1]);
  printf("  Drifts in Z dimension: %d\n", drifts[2]);
  printf("  Drifts in pitch: %d\n", drifts[3]);
  printf("  Drifts in roll: %d\n", drifts[4]);
  printf("  Drifts in yaw: %d\n", drifts[5]);
  printf("  Images corrected more than once: %d\n", multiples);

  // merely printing aline beginning with "VOXBO WARNING" will ensure that
  // this log is sent to the user

  if ((spikes[0] + spikes[1] + spikes[2] + spikes[3] + spikes[4] + spikes[5] +
       drifts[0] + drifts[1] + drifts[2] + drifts[3] + drifts[4] + drifts[5] +
       multiples) > 0)
    printf("VOXBO WARNING check log file for extreme motion\n");
}

void realign_help() {
  printf("\nVoxBo realign (v%s)\n", vbversion.c_str());
  printf("usage: realign [flags] file [file ...]\n");
  printf("flags:\n");
  printf("    -r <refimage>        refimage is the reference image\n");
  printf("    -n <num>             number of the reference image\n");
  printf("    -o <outfile>         set the name for the realigned file\n");
  printf(
      "    -p <paramfile>       set the name for the param file to be saved\n");
  printf("    -t <tolerance>       tolerance for (in voxels, default %.1f)\n",
         TOLERANCE);
  printf("    -i <maxiter>         max # of iterations (default %d)\n",
         MAXITERATIONS);
  printf(
      "    -s <spikethresh>     spike warning threshold (in voxels, default "
      "%.1f)\n",
      SPIKETHRESH);
  printf(
      "    -d <driftthresh>     drift warning threshold (in voxels, default "
      "%.1f)\n",
      DRIFTTHRESH);
  printf("    -c <file> [file ...] check parameters from each file\n");
  printf("    -q <file> [file ...] summarize parameters from each file\n");
}
