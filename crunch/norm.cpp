
// norm.cpp
// VoxBo spatial normalization (brain warping) module
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

// note that the guts of the underlying code were based on SPM96b
// source code written for MATLAB by John Ashburner

using namespace std;

#include "vbcrunch.h"

class Norm {
 private:
  string imagename;  // name of the image to be normalized
  string refname;    // name of the reference image
  string paramname;  // name of param file to read or write
  string outname;    // name of file to write
  string zname;      // file with the z dimension we want

  tokenlist args;  // structure to be used for argument parsing
  enum { calc, apply } mode;
  Matrix affine, dims, MF, transform;
  Matrix bb;
  RowVector vox;
  int affineonly;

 public:
  Norm(int argc, char **argv);
  ~Norm();
  void init();
  void guessvoxandbb(double v0, double v1, double v2);
  int Crunch();
  int CalcParams();
  int WriteParams();
  int ReadParams();
  int NormalizeFile();
  void SetOutName();
  void SetParamsName();
  int Norm3D(const string &filename);
  int Norm4D(const string &filename);
};

void norm_help();

int main(int argc, char *argv[]) {
  tzset();         // make sure all times are timezone corrected
  if (argc < 2) {  // not enough args, display autodocs
    norm_help();
    exit(0);
  }

  Norm *nn = new Norm(argc, argv);  // init norm object, parse args
  if (!nn) {
    printf("norm error: couldn't allocate a tiny Norm structure\n");
    exit(5);
  }
  int err = nn->Crunch();  // do the crunching
  delete nn;               // clean up
  exit(err);
}

void Norm::init() {
  mode = apply;
  bb.resize(2, 3);
  vox.resize(3);
  // defaults
  bb(0, 0) = 0;
  bb(1, 0) = 0;
  bb(0, 1) = 0;
  bb(1, 1) = 0;
  bb(0, 2) = 0;
  bb(1, 2) = 0;
  vox(0) = 0;
  vox(1) = 0;
  vox(2) = 0;
  imagename = "";
  refname = "";
  paramname = "";
  outname = "";
  affineonly = 0;
}

void Norm::guessvoxandbb(double v0, double v1, double v2) {
  // if we don't have voxel sizes, use the one passed (from the first file)
  if (vox(0) == 0) {
    vox(0) = v0;
    vox(1) = v1;
    vox(2) = v2;
  }
  if (zname.size()) {
    Cube cb;
    cb.ReadFile(zname);
    if (cb.data_valid) {
      // vox(0)=cb.voxsize[0];
      // vox(1)=cb.voxsize[1];
      vox(2) = cb.voxsize[2];
    } else
      printf("norm: warning: 3D file specified with -z couldn't be read\n");
  }
  // if we don't have a bounding box, make an educated guess
  if (bb(0, 0) == 0) {
    bb(0, 0) = -floor(76.0 / vox(0)) * vox(0);
    bb(1, 0) = floor(77.0 / vox(0)) * vox(0);
    bb(0, 1) = -floor(114.0 / vox(1)) * vox(1);
    bb(1, 1) = floor(77.0 / vox(1)) * vox(1);
    bb(0, 2) = -floor(45.0 / vox(2)) * vox(2);
    bb(1, 2) = floor(87.0 / vox(2)) * vox(2);
  }
}

Norm::Norm(int argc, char *argv[]) {
  init();
  for (int i = 1; i < argc; i++) {
    if (dancmp(argv[i], "-calc")) {
      mode = calc;
    } else if (dancmp(argv[i], "-apply")) {
      mode = apply;
    } else if (dancmp(argv[i], "-a")) {
      affineonly = 1;
    } else if (dancmp(argv[i], "-r")) {
      if (i < argc - 1) {
        refname = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-bb")) {
      if (i < argc - 6) {
        bb(0, 0) = strtod(argv[i + 1], NULL);
        bb(1, 0) = strtod(argv[i + 2], NULL);
        bb(0, 1) = strtod(argv[i + 3], NULL);
        bb(1, 1) = strtod(argv[i + 4], NULL);
        bb(0, 2) = strtod(argv[i + 5], NULL);
        bb(1, 2) = strtod(argv[i + 6], NULL);
        i += 6;
      }
    } else if (dancmp(argv[i], "-v")) {
      if (i < argc - 3) {
        vox(0) = strtod(argv[i + 1], NULL);
        vox(1) = strtod(argv[i + 2], NULL);
        vox(2) = strtod(argv[i + 3], NULL);
        i += 3;
      }
    } else if (dancmp(argv[i], "-p")) {
      if (i < argc - 1) {
        paramname = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-z")) {
      if (i < argc - 1) {
        zname = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-o")) {
      if (i < argc - 1) {
        outname = argv[i + 1];
        i++;
      }
    } else if (dancmp(argv[i], "-i")) {
      if (i < argc - 1) {
        args.Add(argv[i + 1]);
        i++;
      }
    } else if (argv[i][0] != '-') {
      args.Add(argv[i]);
    }
  }
}

Norm::~Norm() {}

int Norm::Crunch() {
  int err;
  switch (mode) {
    case calc:
      if ((err = CalcParams())) return err;
      err = WriteParams();
      return err;
      break;
    case apply:
      if (ReadParams())
        err = NormalizeFile();
      else {
        printf("Couldn't read parameters from %s.\n", paramname.c_str());
        err = 5;
      }
      return err;
      break;
  }
  return 1;  // shouldn't happen
}

int Norm::CalcParams() {
  int errcnt = 0;
  for (int i = 0; i < args.size(); i++) {
    CrunchCube image, ref;
    imagename = args[i];
    ref.ReadFile(refname);
    if (!ref.data_valid) {
      printf("norm: error: couldn't load reference image %s, aborting\n",
             refname.c_str());
      return (1);
    }
    image.ReadFile(imagename);
    if (!image.data_valid) {
      printf("norm: error: couldn't load image %s\n", imagename.c_str());
      errcnt++;
      continue;
    }
    printf("norm: calculating norm params for %s\n", imagename.c_str());
    printf("norm: using reference image %s\n", refname.c_str());

    // FIXME does the following fix the problem?
    if (image.origin[0] == 0 && image.origin[1] == 0 && image.origin[2] == 0) {
      int o1 = image.dimx;
      int o2 = image.dimy;
      int o3 = image.dimz;
      guessorigin(o1, o2, o3);
      image.SetOrigin(o1, o2, o3);
    }

    dan_sn3d(&ref, &image, affine, dims, transform, MF);
    printf("norm: Done calculating.\n");
  }
  return errcnt;
}

int Norm::Norm3D(const string &filename) {
  CrunchCube image, *newimage;
  image.ReadFile(filename);  // read in the image to be normed
  if (!image.data_valid) {
    return (100);  // error
  }
  guessvoxandbb(image.voxsize[0], image.voxsize[1], image.voxsize[2]);
  if (image.origin[0] == 0 && image.origin[1] == 0 && image.origin[2] == 0) {
    int o1 = image.dimx;
    int o2 = image.dimy;
    int o3 = image.dimz;
    guessorigin(o1, o2, o3);
    image.SetOrigin(o1, o2, o3);
  }

  // FIXME verify that all this monkeying with origins works
  image.origin[0]++;
  image.origin[1]++;
  image.origin[2]++;
  newimage =
      dan_write_sn(&image, affine, dims, transform, MF, bb, vox, 5, affineonly);
  // construct the user header for newimage
  newimage->header = image.header;
  // FIXME below lines removed due to odd ob1 origins
  // newimage->origin[0]--;
  // newimage->origin[1]--;
  // newimage->origin[2]--;

  struct tm *mytm;  // when normed
  time_t mytime;    // "
  char tmp[STRINGLEN], timestring[STRINGLEN];

  mytime = time(NULL);
  mytm = localtime(&mytime);
  strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
  sprintf(tmp, "Normalized by norm:\t%s", timestring);
  newimage->AddHeader(tmp);
  sprintf(tmp, "Normalization parameters:\t%s", paramname.c_str());
  newimage->AddHeader(tmp);
  sprintf(tmp, "Original voxel sizes:\t%.4f\t%.4f\t%.4f", image.voxsize[0],
          image.voxsize[1], image.voxsize[2]);
  newimage->AddHeader(tmp);
  sprintf(tmp, "Bounding box:\t%.4f %.4f %.4f %.4f %.4f %.4f", bb(0, 0),
          bb(1, 0), bb(0, 1), bb(1, 1), bb(0, 2), bb(1, 2));
  newimage->AddHeader(tmp);

  newimage->SetFileName(outname);
  newimage->SetFileFormat(image.GetFileFormat());
  newimage->WriteFile();

  delete newimage;
  return (0);  // no error!
}

int Norm::NormalizeFile() {
  int errcount = 0;
  for (int i = 0; i < args.size(); i++) {
    imagename = args[i];
    SetOutName();
    SetParamsName();
    printf("norm: Normalizing %s\n", imagename.c_str());
    printf("norm: Parameter file %s\n", paramname.c_str());
    Tes ts;
    Cube cb;
    if (ts.ReadHeader(imagename) == 0) {
      if (Norm4D(imagename)) errcount++;
    } else if (cb.ReadHeader(imagename) == 0) {
      if (Norm3D(imagename)) errcount++;
    } else {
      printf("norm: error: invalid filetype for file %s\n", imagename.c_str());
    }
    printf("norm: Done normalizing %s.\n", imagename.c_str());
  }
  return (errcount);
}

void Norm::SetOutName() {
  if (!outname.size())
    outname = xdirname(imagename) + (string) "/n" + xfilename(imagename);
}

void Norm::SetParamsName() {
  if (paramname.size())  // user gave us one, great!
    return;
  // user wants default, based on image name
  char tmp[STRINGLEN], *ext;
  ext = strrchr(tmp, '.');
  if (ext)  // chop off extension if there is one
    *ext = '\0';
  paramname = (string)tmp + "_MoveParams.ref";
}

int Norm::Norm4D(const string &filename) {
  CrunchCube *newcube;
  Tes *mytes, *newtes;
  char tmp[STRINGLEN], timestring[STRINGLEN];
  int o1, o2, o3;
  struct tm *mytm;  // when normed
  time_t mytime;    // "

  mytes = new Tes;
  mytes->ReadFile(filename);
  newtes = (Tes *)NULL;
  if (!mytes->data_valid) {
    printf("norm: invalid tes file: %s\n", filename.c_str());
    delete mytes;
    return 200;  // panic and exit
  }
  guessvoxandbb(mytes->voxsize[0], mytes->voxsize[1], mytes->voxsize[2]);
  // are we going to use o1o2o3?
  if (mytes->origin[0] == 0 && mytes->origin[1] == 0 && mytes->origin[2] == 0) {
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

  int interval = (int)(mytes->dimt / 20);
  if (interval < 1) interval = 1;
  for (int i = 0; i < mytes->dimt; i++) {
    CrunchCube mycube;
    mytes->getCube(i, mycube);
    // now normalize it
    mycube.origin[0]++;
    mycube.origin[1]++;
    mycube.origin[2]++;
    newcube = dan_write_sn(&mycube, affine, dims, transform, MF, bb, vox, 5,
                           affineonly);
    // FIXME below lines removed due to odd origins, should be double-checked
    // newcube->origin[0]--;
    // newcube->origin[1]--;
    // newcube->origin[2]--;
    if (!newtes) {  // if we haven't allocated the new tes yet
      newtes = new Tes();
      newtes->SetVolume(newcube->dimx, newcube->dimy, newcube->dimz,
                        mytes->dimt, newcube->datatype);
      newtes->SetVoxSizes(vox(0), vox(1), vox(2));
      newtes->origin[0] = newcube->origin[0];
      newtes->origin[1] = newcube->origin[1];
      newtes->origin[2] = newcube->origin[2];
      if (!newtes->data) {
        delete mytes;
        delete newcube;
        printf(
            "norm: error: couldn't allocate space for the normalized image\n");
        return (1);
      }
      newtes->header = mytes->header;
    }
    newtes->SetCube(i, newcube);  // bang it back into the tes
    delete newcube;
    if ((i + 1) % interval == 0)
      printf("Percent done: %d\n", (i * 100) / mytes->dimt);
    fflush(stdout);
  }
  mytime = time(NULL);
  mytm = localtime(&mytime);
  strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
  sprintf(tmp, "Normalized by norm:\t%s", timestring);
  newtes->AddHeader(tmp);
  sprintf(tmp, "Normalization parameters:\t%s", paramname.c_str());
  newtes->AddHeader(tmp);
  sprintf(tmp, "Original voxel sizes:\t%.4f\t%.4f\t%.4f", mytes->voxsize[0],
          mytes->voxsize[1], mytes->voxsize[2]);
  newtes->AddHeader(tmp);
  sprintf(tmp, "Bounding box:\t%.4f %.4f %.4f %.4f %.4f %.4f", bb(0, 0),
          bb(1, 0), bb(0, 1), bb(1, 1), bb(0, 2), bb(1, 2));
  newtes->AddHeader(tmp);

  newtes->SetFileName(outname);
  newtes->SetFileFormat(mytes->GetFileFormat());
  int err = newtes->WriteFile();

  delete mytes;
  delete newtes;

  return (err);
}

int Norm::ReadParams() {
  int m, n, trows, tcols, i;

  affine.resize(4, 4);
  dims.resize(6, 3);
  MF.resize(4, 4);

  VB_Vector vv(paramname);
  if (!vv.valid) return (FALSE);
  i = 0;
  for (n = 0; n < 4; n++)
    for (m = 0; m < 4; m++) {
      affine(m, n) = vv[i++];
    }
  for (n = 0; n < 3; n++)
    for (m = 0; m < 6; m++) {
      dims(m, n) = vv[i++];
    }
  for (n = 0; n < 4; n++)
    for (m = 0; m < 4; m++) {
      MF(m, n) = vv[i++];
    }
  trows = (int)vv[i++];
  tcols = (int)vv[i++];
  transform.resize(trows, tcols);
  for (n = 0; n < (int)tcols; n++)
    for (m = 0; m < (int)trows; m++) {
      transform(m, n) = vv[i++];
    }
  return TRUE;
}

int Norm::WriteParams() {
  int totallen, m, n, trows, tcols;
  char timestring[STRINGLEN];
  FILE *fp;
  struct tm *mytm;  // when normed
  time_t mytime;    // "

  // doesn't use standard vec writing code, just rolls its own
  if ((fp = fopen(paramname.c_str(), "w")) == NULL) {
    printf("norm: couldn't write parameter file %s\n", paramname.c_str());
    return (100);
  }
  fprintf(fp, ";VB98\n;REF1\n;\n");
  mytime = time(NULL);
  mytm = localtime(&mytime);
  strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
  fprintf(fp, "; Normalization parameters calculated %s\n", timestring);
  fprintf(fp, "; Template image: %s\n", refname.c_str());
  fprintf(fp, "; Image image: %s\n", imagename.c_str());

  // total size of param file should be 16 + 18 + 16 + 2 + len(transform)
  totallen = 16 + 18 + 16 + 2 + (transform.rows() * transform.cols());
  for (n = 0; n < 4; n++)
    for (m = 0; m < 4; m++) fprintf(fp, "%.8f\n", (double)affine(m, n));
  for (n = 0; n < 3; n++)
    for (m = 0; m < 6; m++) fprintf(fp, "%.8f\n", (double)dims(m, n));
  for (n = 0; n < 4; n++)
    for (m = 0; m < 4; m++) fprintf(fp, "%.8f\n", MF(m, n));
  trows = (int)transform.rows();
  tcols = (int)transform.cols();
  fprintf(fp, "%d\n", transform.rows());
  fprintf(fp, "%d\n", transform.cols());
  transform.resize((int)trows, (int)tcols);
  for (n = 0; n < tcols; n++)
    for (m = 0; m < trows; m++) fprintf(fp, "%.8f\n", (double)transform(m, n));
  fclose(fp);
  return (0);  // no error!
}

void norm_help() {
  printf("\nVoxBo norm (v%s)\n", vbversion.c_str());
  printf("usage: norm [-calc/-apply] [flags] file [file ...]\n");
  printf("flags:\n");
  printf("   -r <refimage>       refimage is the reference image\n");
  printf("   -o <outfile>        set the name for the normalized data\n");
  printf(
      "   -p <paramfile>      set the name for the param file to be "
      "read/written\n");
  printf("   -v <v1> <v2> <v3>   desired voxel sizes\n");
  printf("   -z <filename>       filename with desired z voxel size\n");
  printf("   -bb <x x y y z z >  bounding box\n");
  printf("   -a                  affine transformation only\n");
  printf("examples:\n");
  printf("To calculate params:\n");
  printf("  norm -calc -r template.cub -p myparams.ref myanatomy.cub\n");
  printf("To apply those params\n");
  printf("  norm -apply -o nfunc.cub -p myparams.ref func.cub\n");
  printf("\n");
}
