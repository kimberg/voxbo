
// gds_main.cpp
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
// original version written by Dongbo Hu

using namespace std;

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include "gds.h"

void gds_help();
void gds_make_sample(const string &);
void gds_show_sample();
void gds_sample(FILE *);

VBPrefs vbp;
/* Main function for gds */
int main(int argc, char **argv) {
  tokenlist args;
  string fileName;
  bool validFlag = false;
  vbp.init();

  args.Transfer(argc - 1, argv + 1);
  if (args.size() == 0) {
    gds_help();
    exit(0);
  }

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      gds_show_sample();
      exit(0);
    }
    if (args[i] == "-x") {
       if (args.size() > i+1)
          gds_make_sample(args[i + 1]);
       else
          printf("[E] gds -x needs a filename to store the sample gds script file\n");
       exit(0);
    }
    if (args[i] == "-v") {
      if (args.size() > i + 1) {
        validFlag = true;
        fileName = args[i + 1];
        break;
      }
      else {
        printf("[E] gds -v needs an input gds script filename\n");
        exit(0);
      }
    }
    else fileName = args[i];
  }

  scriptReader myG;
  myG.validateOnly = validFlag;
  //   if (myG.parseFile(fileName) && !validFlag)
  //     myG.makeAllG();
  myG.parseFile(fileName);
  
  return 0;
}

/* gds_help() shows usage message */
void gds_help()
{
  printf("VoxBo gds (v%s)\n", vbversion.c_str());
  printf("usage: gds [flags] file\n");
  printf("flags:\n");
  printf("    -h            i'm lazy, show me a sample script file\n");
  printf("    -x <filename> i'm really lazy, make me a sample script file\n");
  printf("    -v <filename> don't generate the matrix, only check input file's validity\n");
}

void gds_make_sample(const string &fname)
{
  printf("Creating sample script file in %s ... ", fname.c_str());  
  fflush(stdout);
  FILE *fp = fopen(fname.c_str(), "w");
  if (fp) {
    gds_sample(fp);
    fclose(fp);
    printf("done.\n");
  }
  else {
    printf("failed.\n");
  }
}

/* gds_show_sample() will print out a sample file on output screen */
void gds_show_sample()
{
  printf("###############################################################\n");
  printf("# Sample script file for VoxBo gds.\n");
  printf("###############################################################\n");
  printf("# Any line that starts with # is treated as comment\n");
  printf("# Replace anything inside < > to the real value\n\n");

  printf("# gsession starts a new G design session, followed by the analysis folder's name\n");
  printf("# ALWAYS put gsession at the beginning of a new G design session\n"); 
  printf("gsession /tmp/foo\n\n");
  printf("###############################################################\n");
  printf("# Then define some common variables required by each gsession\n");
  printf("###############################################################\n");
  printf("# scan defines the tes file(s), not required if TR and length are defined\n");
  printf("# One file each line, multiple lines allowed\n");
  printf("scan /datadir/foo.tes\n");
  printf("scan /datadir/bar.tes\n\n");

  printf("# Fake scan can be defined as:\n");
  printf("scan foo1 100\n");
  printf("# This defines a fake tes file whose name is \"foo1\" and length is 100\n");
  printf("# When fake scan is defined, global signal or movement parameter covariates\n");
  printf("# can NOT be added because GS or MoveParams ref files are not available.\n\n");

  printf("# TR in unit of ms, not required if scan is defined\n");
  printf("# If multiple lines are available, only the last one counts\n"); 
  printf("TR 2000\n\n");

  printf("# Up-sampling rate in unit of ms, optional, default is 100\n");
  printf("# If multiple lines are available, only the last one counts\n"); 
  printf("sampling 100\n\n");
 
  printf("# Total number of time points, not required if scan is defined\n");
  printf("length 320\n\n");

  printf("# Specify condition function file, required by diagonal set or contrasts\n");
  printf("# If multiple lines are available, only the last one counts\n"); 
  printf("condition /datadir/condition.ref\n");  
  printf("# Customize condition function key labels from a certain txt file\n");
  printf("# If multiple lines are available, only the last one counts\n"); 
  printf("condition-label-file /datadir/label.txt\n");
  printf("# Customize condition key labels by whatever string you like\n");
  printf("# Multiple lines are allowed\n");
  printf("# NOTE: condition-label-file and condition-label-name are exclusive tags\n");
  printf("condition-label-name baseline rest move ...\n");
  printf("condition-label-name condition7 condition8 ...\n\n");

  printf("# mean-center-all will mean center all non-intercept covariates\n");
  printf("# before saving them in G and preG files. This option is on by default.\n");
  printf("# Remove or comment out this line if you don't want it.\n");
  printf("mean-center-all\n\n");

  printf("# Section is used to create/modify covariates or check efficiency.\n");
  printf("# Each section starts with any one of the four tags:\n");
  printf("# (1) newcov (add new covariates);\n");
  printf("# (2) modcov (modify a selected covariate);\n");
  printf("# (3) modcov+ (modify a selected covariate and create new ones besed on the selected one);\n");
  printf("# (4) chkeff (check covariate efficiency and remove those whose efficiency \n");
  printf("# is less than a certain numeric value\n\n");

  printf("# Four tags are allowed in the body of newcov/modcov/modcov+ to modify covariate's properties:\n");
  printf("# (1) cov-name specifies new covariate's name or modify selected one's name);\n");
  printf("# cov-name line can be put anywhere in a section's body.\n");
  printf("# The number of defined names doesn't have to be equal to that of new covariate(s).\n");
  printf("# Multiple cov-name lines are also allowed\n");
  printf("cov-name foo bar ...\n"); 
  printf("cov-name cov99\n");
  printf("# (2) type specifies new covariate's type or modify selected one's type;\n");
  printf("# \"type\" needs an argument as covariate type. It could be: \n");
  printf("# \"I\": Interest, \"N\": NoInterest, \"K\": KeepNoInterest\n");
  printf("# (3) group puts covariate in certain group\n");
  printf("# group needs a string argument as group name.\n");
  printf("# (4) option matches the modification actions on GLM interface.\n");
  printf("# It is the most important (and complicated) part of gds script.\n");
  printf("# It modifies the vector that represents each covariate.\n");
  printf("# NOT every option is valid in any section, some are valid ONLY in certain sections\n\n");

  printf("# Nine options are allowed in newcov/modcov/modcov+ include:\n");
  printf("# (1) mean-center (\"Modify Selection(s)->Mean center\" on GLM interface);\n");
  printf("# (2) mean-center-non-zero (\"Modify Selection(s)->Mean center nonzero\" on GLM interface);\n");
  printf("# (3) unit-variance (\"Modify Selection(s)->Unit variance\" on GLM interface);\n");
  printf("# (4) unit-excursion (\"Modify Selection(s)->Unit excursion\" on GLM interface);\n");
  printf("# (5) convert-delta (\"Modify Selection(s)->Convert to delta\" on GLM interface);\n");
  printf("# (6) convolve (\"Modify Selection(s)->Convolve\" on GLM ionterface);\n");
  printf("# \"convolve\" accepts two or three arguments:\n"); 
  printf("option convolve <kernel_file_path> <kernel_TR> <optional_tag>\n");
  printf("# (7) time-shift (\"Modify Selection(s)->Time shift\" on GLM interface);\n");
  printf("# This option requires an argument as time shift value in unit of ms:\n");
  printf("option time-shift 2000\n");
  printf("# (8) multiply (\"Modify Selection(s)->Multiply\" on GLM ionterface);\n");
  printf("# It needs an argument as the covariate that is used for multiplication:\n");
  printf("option multiply <existing covariate's fullname>\n");
  printf("# (9) orthog (\"Modify Selection(s)->Orthogonalize\"on GLM interface);\n");
  printf("# One of the two tags are required to specify covariates involved in this action:\n");
  printf("# orth-type/orth-name\n");
  printf("# orth-type specifies the covariate type in orthognaliztion;\n");
  printf("# acceptable options for orth-type are:\n");
  printf("# A (all covariates), I (Intereset), N (NoInterest), K (KeepNoInterest)\n");
  printf("# orth-name specifies the covariate name(s) in orthognalization;\n");
  printf("# Note that orth-type and orth-name are exclusive tags\n");
  printf("# example 1: orthognalization based on all covariates:\n");
  printf("option orthog\n");
  printf("  orth-type A\n");
  printf("# example 2: orthognalization based on two covariates: foo and bar\n");
  printf("option orthog\n");
  printf("  orth-name foo bar\n");
  printf("# or:\n");
  printf("option orthog\n");
  printf("  orth-name foo\n");
  printf("  orth-name bar\n");
  printf("# Note: adding option lines in newcov option is equivalent to creating\n");
  printf("# a separate modcov section and inserting options lines in modcov body\n\n"); 

  printf("# Three options not only modifies a selected covariate, but also add new covariate\n");
  printf("# based on the selected one. They are ONLY allowed in modcov+ section and\n");
  printf("# each option is allowed to be called only ONCE in a certain section.\n");
  printf("# These options are:\n");
  printf("# (1) eigen-vector (\"Modify Selection(s)->Eigenvector set\" on GLM interface);\n"); 
  printf("# This option does not need any argument:\n");
  printf("# option eigen-vector\n");
  printf("# (2) fir (\"Modify Selection(s)->Finite impulse response\" on GLM interface);\n");
  printf("# This option needs an argument as # of TR:\n");
  printf("# option fir <# of TR>\n");
  printf("# (3) fourier-set (\"Modify Selection(s)->Fourier set\" on GLM interface);\n");
  printf("# This option does not need any argument.\n");
  printf("# Four tags are required to set up Fourier set parameters:\n");
  printf("# (a) fs-period expects a positive integer an period to model\n");
  printf("# (b) fs-harmonics expects a positive integer as the number of harmonics\n");
  printf("# (c) fs-zero-freq expects y/yes or n/no to decide wether frequency zero should be added or not\n");
  printf("# (d) fs-alter-cov expects y/yes or n/no to decide whether covariate will be altered or not\n");
  printf("# A valid fourier-set option will look like:\n");
  printf("option fourier-set\n");  
  printf("  fs-period 5\n");
  printf("  fs-harmonics 3\n");
  printf("  fs-zero-freq y\n");
  printf("  fs-alter-cov n\n");

  printf("# Two options generate a new covariate based on selected one.\n");
  printf("# They are ONLY allowed in \"newcov cp\" section and each option\n");
  printf("# is allowed to be called only ONCE in the section. These options are:\n");
  printf("# (1) exponential (\"Modify Selection(s)->Exponential\" on GLM interface);\n");
  printf("# exponential option needs an argument as exponential value:\n");
  printf("option exponential 2.3\n");
  printf("# (2) derivative (\"Modify Selection(s)->Derivative\" on GLM interface);\n");
  printf("# derivative needs a positive integer argument as the number of derivative(s):\n");
  printf("option derivative 2\n\n");

  printf("# \"end\" tag closes previous section\n");

  printf("# \"newcov\" section creates new covariate(s)\n");
  printf("# Actions that are supported in this section include:\n");
  printf("# single, intercept, trial-effect, diagonal, contrast, var-trialfx\n");
  printf("# scan-effect, global-signal, move-params, spike, txt-file, cp\n\n");

  printf("# (1) \"newcov single <ref filename>\" adds single covariate based on ref file;\n");
  printf("# It needs a string argument as ref filename.\n"); 
  printf("# Default covariate name is \"undefined\"\n");
  printf("# NOTE: if cov-name is available in this section, this name will be overwritten\n");

  printf("newcov single /my_dir/foo.ref\n");
  printf("  # Default type is I for single covariate, so the next line is optional\n");
  printf("  type I\n");
  printf("  # Name it foo instead of undefined\n");
  printf("  cov-name foo\n"); 

  printf("  # do mean-center and convolve on it\n");
  printf("  option mean-center\n");
  printf("  option convolve <kernel_file_path> <kernel_TR> <optional_tag>\n");
  printf("# Close the section by \"end\"\n");
  printf("end\n\n");

  printf("# (2) \"newcov intercept\" adds intercept covariate\n");
  printf("# It takes an optional argument as covariate name\n");
  printf("# default name: Intercept, default type: K\n");
  printf("# cov-name will overwrite name\n");
  printf("newcov intercept\n");
  printf("# This line sets type to \"NoInterest\"\n");
  printf("type N\n");
  printf("end\n\n");

  printf("# (3) \"newcov diagonal\" adds diagonal set\n");
  printf("# It doesn't need any arguments\n");
  printf("# default name: condition key name, default type: I\n");
  printf("newcov diagonal\n");
  printf("# I want to set the type to be NoInterest instead\n");
  printf("  type N\n");
  printf("  # \"scale\" tag sets \"scale by trial count\" option in\n");
  printf("  # \"newcov diagonal\" and \"newcov contrast\" sections.\n");
  printf("  # So it is allowed ONLY in these two sections.\n");
  printf("  # Set \"scale by trial count\" to be yes (default is n/no)\n");
  printf("  scale y\n");
  printf("  # \"center-norm\" tag sets \"center and normalize\" option in\n");
  printf("  # \"newcov diagonal\" and \"newcov contrast\" sections.\n");
  printf("  # So it is allowed ONLY in these two sections.\n");
  printf("  # Set \"center and normalize\" to be No (default is y/yes)\n");
  printf("  center-norm n\n");
  printf("end\n\n");

  printf("# (4) \"newcov contrast\" adds contrast covaraite(s)\n");
  printf("# It doesn't need any arguments\n");
  printf("# default name: \n");
  printf("# <numberic value #1>*<condition key #1> + <numberic value #2>*<condition key #2> + ...\n");
  printf("# default type: I\n");
  printf("newcov contrast\n");  
  printf("  # matrix-row is a tag allowed ONLY in \"newcov contrast\" section.\n");
  printf("  # it defines a certain row of contrast matrix\n");
  printf("  # The number of arguments of matrix-row must be equal to the number of condition keys\n");
  printf("  matrix-row 3 5\n");
  printf("  matrix-row 1.3 4.77\n");
  printf("  # Do you want \"scale by trial count\" option? The default is y/yes\n"); 
  printf("  scale y\n");
  printf("  # Do you want \"center and normalize\" option? The default is y/yes\n"); 
  printf("  center-norm n\n");
  printf("  # put them into a group called contrast01\n");
  printf("  group contrast01\n");
  printf("  # Give them some easy-to-remember names\n");
  printf("  cov-name xxx yyy\n");
  printf("end\n\n");

  printf("# (5) \"newcov trial-effect <trial_length>\" adds trial effect covaraite(s)\n");
  printf("# <trial_length> is the length of trial effect (unit: second)\n");
  printf("# default type: I; default name: trialfx-<index>\n");
  printf("# e.g. the covariates created in this section will be called:\n");
  printf("# trialfx-1, trialfx-2, ...\n");
  printf("newcov trial-effect 40\n");
  printf("  # I want to shift 3 seconds (3000ms) on each new trial effect covariate\n");
  printf("  option time-shift 3000\n");
  printf("end\n\n");

  printf("# (6) \"newcov var-trialfx <trial filename>\" adds var length trial effect covaraite(s)\n");
  printf("# <trial filename> is the file that includes var length\n");
  printf("# default type: I; default name: var-trialfx-<index>\n");
  printf("newcov var-trialfx /home/foo/trial.ref\n");
  printf("  type n\n");
  printf("end\n\n");

  printf("# (7) \"newcov scan-effect\" adds scan effect. No arguments needed\n");
  printf("# default type: N\n; default name: scanfx-<index>");
  printf("newcov scan-effect\n");
  printf("scan-length xxx yyy\n");
  printf("end\n");
  printf("# scan-length line is required if no tes files are defined earlier\n\n");


  printf("# (8) \"newcov global-signal\" adds global signal covariate. No arguments needed\n");
  printf("# default type: N; default name: global-signal-<index>\n");
  printf("newcov global-signal\n");
  printf("end\n\n");

  printf("# (9) \"newcov move-params\" adds movement parameter covariates. No arguments needed\n");
  printf("# default type: N; default name: move-params-<scan index>-X/Y/Z/pitch/roll/yaw\n");
  printf("newcov move-params\n");
  printf("end\n\n");

  printf("# (10) \"newcov spike <spike position>\" adds spike covariates.\n");
  printf("# It needs an array of integers as argument to define the spike position in unit of TR\n");
  printf("# default type: N; default name: spike-<spike position>\n");
  printf("# Note that by default spike covariates are always mean-center'ed\n");
  printf("newcov spike \"10, 20, 67-70\"\n");
  printf("type I\n");
  printf("end\n");
  printf("# Or:\n");
  printf("newcov spike\n");
  printf("absolute \"10, 20, 67-70\"\n");
  printf("type I\n");
  printf("end\n\n");

  printf("# \"newcov spike\" section can also create spikes whose positions are relative\n"); 
  printf("# to a certain real or fake tes file, for example:\n");
  printf("newcov spike\n");
  printf("absolute 10-12\n");
  printf("relative foo1 \"2,5\"\n"); 
  printf("end\n");
  printf("# The \"absolute\" line creates spike covariates at absolute position of 10, 11 and 12;\n");
  printf("# the \"relative\" line creates 2 spike covariates relative to covariate \"foo1\"\n");
  printf("# \"relative\" accepts both tes file name and tes file index.\n");
  printf("# If \"foo1\" has index of 2, (the 3rd tes file), the section above is equivalent to:\n");
  printf("newcov spike\n");
  printf("absolute 10-12\n");
  printf("relative 2 \"2,5\"\n"); 
  printf("end\n\n");
  
  printf("# (11) \"newcov txt-file <txt filename>\" reads an input txt file and\n");
  printf("# loads each column as a seperate covariate.\n");
  printf("# The argument is txt filename\n");
  printf("# default type: N, default name: txtVar-<column #>\n");
  printf("newcov txt-file /tmp/foo.txt\n");
  printf("end\n\n");

  printf("# (12) \"newcov cp <covariate name>\" duplicates a covariate that already exists.\n");
  printf("# <covariate_name> is name of the covariate that will be duplicated\n");
  printf("# Note that this section supports two exclusive options:\n");
  printf("# derivative and exponential\n");
  printf("newcov cp <covariate_name>\n");
  printf("  option derivative 2\n");
  printf("end\n\n");

  printf("# \"modcov <covariate name>\" selects a certain covariate by its name and modify\n"); 
  printf("its properties such as name, group, type, or the vector.\n");
  printf("# Note again that it only supports the options that do NOT create new covariate(s).\n");
  printf("modcov <covariate name>\n");
  printf("cov-name <new covariate name>\n");
  printf("group <new group name>\n");
  printf("type k\n");
  printf("option convert-delta\n");
  printf("option ...\n");
  printf("end\n\n");

  printf("# \"modcov+ <covariate name>\" is similar to modcov,\n");
  printf("# but it also supports three exclusive options:\n");
  printf("# eigen-vector, fir, fourier-set\n");
  printf("modcov+ <covariate name>\n");
  printf("type n\n");
  printf("  option eigen-vector\n");
  printf("end\n\n");

  printf("# \"del-cov <covariate name>\" removes a certain covariate\n");
  printf("# <covariate name> is the covariate's FULL name\n");
  printf("del-cov group1->foo\n\n");

  printf("# \"del-all-cov\" removes all covariates\n");
  printf("del-all-cov\n\n");

  printf("# \"openG <G matrix file>\" loads a G matrix file that already exists.\n");
  printf("# If a preG file (upsampled version of G file) with same header coexists\n");
  printf("# in the same directory, the preG file will be loaded instead.\n");
  printf("# When it is loaded, the G/preG file's header is always read first to get TR,\n");
  printf("# of images and condition function. If these parameters in the header are\n");
  printf("# different from previous definition, the latter will be overwritten.\n");
  printf("openG foo.G\n\n");
  
  printf("# chkeff section is designed for efficiency check. It needs five parameters totally:\n");
  printf("# (1) base covariate name; (2) downsample option; (3) filter filename;\n");
  printf("# (4) type of covariates that will be included for efficiency check; (5) cutoff value\n");
  printf("# chkeff is followed by the base covariate's FULL name\n");
  printf("# Base covariate type must match eff-type defined below\n");
  printf("# chkeff myGroup->myName\n");
  printf("  # downsample has three options:\n");
  printf("  # no (no downsample at all),\n");
  printf("  # before (downsample before convolution),\n");
  printf("  # after (downsample after convolution)\n");
  printf("  downsample no/before/after\n");
  printf("  # filter is followed by the path of filter file for efficiency check\n");
  printf("  filter /mypath/foo.ref\n");
  printf("  # eff-type is the covariate type that will be included in efficiency check\n");
  printf("  # three options are acceptable:\n");
  printf("  # A/a (all covariates), I/i (Interest), N/n (NoInterest)\n");
  printf("  # Note that N/n include both NoInterest and KeepNoInterest\n");
  printf("  eff-type A\n");
  printf("  # eff-cutoff is followed by a numeric value.\n");
  printf("  # covariate whose relative efficiency is less than this value wil be deleted\n");
  printf("  eff-cutoff 0.02\n");
  printf("# end is required at the end of chkeff section\n");
  printf("end\n\n");

  printf("# Create another G matrix\n");
  printf("gsession /tmp/bar\n");
  printf(" ... ... \n\n");

  printf("# include <filename> line is equivalent to adding the content of\n");
  printf("# <filename> at the position of this line\n");
  printf("include /data/foo.gds\n\n");

}

/* gds_sample() will write a sample script to the input file */
void gds_sample(FILE *fp)
{
  fprintf(fp, "###############################################################\n");
  fprintf(fp, "# Sample script file for VoxBo gds.\n");
  fprintf(fp, "###############################################################\n");
  fprintf(fp, "# Any line that starts with # is treated as comment\n");
  fprintf(fp, "# Replace anything inside < > to the real value\n\n");

  fprintf(fp, "# gsession starts a new G design session, followed by the analysis folder's name\n");
  fprintf(fp, "# ALWAYS put gsession at the beginning of a new G design session\n"); 
  fprintf(fp, "gsession /tmp/foo\n\n");
  fprintf(fp, "###############################################################\n");
  fprintf(fp, "# Then define some common variables required by each gsession\n");
  fprintf(fp, "###############################################################\n");
  fprintf(fp, "# scan defines the tes file(s), not required if TR and length are defined\n");
  fprintf(fp, "# One file each line, multiple lines allowed\n");
  fprintf(fp, "scan /datadir/foo.tes\n");
  fprintf(fp, "scan /datadir/bar.tes\n\n");

  fprintf(fp, "# Fake scan can be defined as:\n");
  fprintf(fp, "scan foo1 100\n");
  fprintf(fp, "# This defines a fake tes file whose  name is \"foo1\" and length is 100\n");
  fprintf(fp, "# When fake scan is defined, global signal or movement parameter covariates\n");
  fprintf(fp, "# can NOT be added because GS or MoveParams ref files are not available.\n\n");

  fprintf(fp, "# TR in unit of ms, not required if scan is defined\n");
  fprintf(fp, "# If multiple lines are available, only the last one counts\n"); 
  fprintf(fp, "TR 2000\n\n");

  fprintf(fp, "# Up-sampling rate in unit of ms, optional, default is 100\n");
  fprintf(fp, "# If multiple lines are available, only the last one counts\n"); 
  fprintf(fp, "sampling 100\n\n");
 
  fprintf(fp, "# Total number of time points, not required if scan is defined\n");
  fprintf(fp, "length 320\n\n");

  fprintf(fp, "# Specify condition function file, required by diagonal set or contrasts\n");
  fprintf(fp, "# If multiple lines are available, only the last one counts\n"); 
  fprintf(fp, "condition /datadir/condition.ref\n");  
  fprintf(fp, "# Customize condition function key labels from a certain txt file\n");
  fprintf(fp, "# If multiple lines are available, only the last one counts\n"); 
  fprintf(fp, "condition-label-file /datadir/label.txt\n");
  fprintf(fp, "# Customize condition key labels by whatever string you like\n");
  fprintf(fp, "# Multiple lines are allowed\n");
  fprintf(fp, "# NOTE: condition-label-file and condition-label-name are exclusive tags\n");
  fprintf(fp, "condition-label-name baseline rest move ...\n");
  fprintf(fp, "condition-label-name condition7 condition8 ...\n\n");

  fprintf(fp, "# mean-center-all will mean center all non-intercept covariates\n");
  fprintf(fp, "# before saving them in G and preG files. This option is on by default.\n");
  fprintf(fp, "# Remove or comment out this line if you don't want it.\n");
  fprintf(fp, "mean-center-all\n\n");

  fprintf(fp, "# Section is used to create/modify covariates or check efficiency.\n");
  fprintf(fp, "# Each section starts with any one of the four tags:\n");
  fprintf(fp, "# (1) newcov (add new covariates);\n");
  fprintf(fp, "# (2) modcov (modify a selected covariate);\n");
  fprintf(fp, "# (3) modcov+ (modify a selected covariate and create new ones besed on the selected one);\n");
  fprintf(fp, "# (4) chkeff (check covariate efficiency and remove those whose efficiency \n");
  fprintf(fp, "# is less than a certain numeric value\n\n");

  fprintf(fp, "# Four tags are allowed in the body of newcov/modcov/modcov+ to modify covariate's properties:\n");
  fprintf(fp, "# (1) cov-name specifies new covariate's name or modify selected one's name);\n");
  fprintf(fp, "# cov-name line can be put anywhere in a section's body.\n");
  fprintf(fp, "# The number of defined names doesn't have to be equal to that of new covariate(s).\n");
  fprintf(fp, "# Multiple cov-name lines are also allowed\n");
  fprintf(fp, "cov-name foo bar ...\n"); 
  fprintf(fp, "cov-name cov99\n");
  fprintf(fp, "# (2) type specifies new covariate's type or modify selected one's type;\n");
  fprintf(fp, "# \"type\" needs an argument as covariate type. It could be: \n");
  fprintf(fp, "# \"I\": Interest, \"N\": NoInterest, \"K\": KeepNoInterest\n");
  fprintf(fp, "# (3) group puts covariate in a certain group\n");
  fprintf(fp, "# group needs a string argument as group name.\n");
  fprintf(fp, "# (4) option matches the modification actions on GLM interface.\n");
  fprintf(fp, "# It is the most important (and complicated) part of gds script.\n");
  fprintf(fp, "# It modifies the vector that represents each covariate.\n");
  fprintf(fp, "# NOT every option is valid in any section, some are valid ONLY in certain sections\n\n");

  fprintf(fp, "# Nine options are allowed in newcov/modcov/modcov+ include:\n");
  fprintf(fp, "# (1) mean-center (\"Modify Selection(s)->Mean center\" on GLM interface);\n");
  fprintf(fp, "# (2) mean-center-non-zero (\"Modify Selection(s)->Mean center nonzero\" on GLM interface);\n");
  fprintf(fp, "# (3) unit-variance (\"Modify Selection(s)->Unit variance\" on GLM interface);\n");
  fprintf(fp, "# (4) unit-excursion (\"Modify Selection(s)->Unit excursion\" on GLM interface);\n");
  fprintf(fp, "# (5) convert-delta (\"Modify Selection(s)->Convert to delta\" on GLM interface);\n");
  fprintf(fp, "# (6) convolve (\"Modify Selection(s)->Convolve\" on GLM ionterface);\n");
  fprintf(fp, "# \"convolve\" accepts two or three arguments:\n"); 
  fprintf(fp, "option convolve <kernel_file_path> <kernel_TR> <optional_tag>\n");
  fprintf(fp, "# (7) time-shift (\"Modify Selection(s)->Time shift\" on GLM interface);\n");
  fprintf(fp, "# This option requires an argument as time shift value in unit of ms:\n");
  fprintf(fp, "option time-shift 2000\n");
  fprintf(fp, "# (8) multiply (\"Modify Selection(s)->Multiply\" on GLM ionterface);\n");
  fprintf(fp, "# It needs an argument as the covariate that is used for multiplication:\n");
  fprintf(fp, "option multiply <existing covariate's fullname>\n");
  fprintf(fp, "# (9) orthog (\"Modify Selection(s)->Orthogonalize\"on GLM interface);\n");
  fprintf(fp, "# One of the two tags are required to specify covariates involved in this action:\n");
  fprintf(fp, "# orth-type/orth-name\n");
  fprintf(fp, "# orth-type specifies the covariate type in orthognaliztion;\n");
  fprintf(fp, "# acceptable options for orth-type are:\n");
  fprintf(fp, "# A (all covariates), I (Intereset), N (NoInterest), K (KeepNoInterest)\n");
  fprintf(fp, "# orth-name specifies the covariate name(s) in orthognalization;\n");
  fprintf(fp, "# Note that orth-type and orth-name are exclusive tags\n");
  fprintf(fp, "# example 1: orthognalization based on all covariates:\n");
  fprintf(fp, "option orthog\n");
  fprintf(fp, "  orth-type A\n");
  fprintf(fp, "# example 2: orthognalization based on two covariates: foo and bar\n");
  fprintf(fp, "option orthog\n");
  fprintf(fp, "  orth-name foo bar\n");
  fprintf(fp, "# or:\n");
  fprintf(fp, "option orthog\n");
  fprintf(fp, "  orth-name foo\n");
  fprintf(fp, "  orth-name bar\n");
  fprintf(fp, "# Note: adding option lines in newcov option is equivalent to creating\n");
  fprintf(fp, "# a separate modcov section and inserting options lines in modcov body\n\n"); 

  fprintf(fp, "# Three options not only modifies a selected covariate, but also add new covariate\n");
  fprintf(fp, "# based on the selected one. They are ONLY allowed in modcov+ section and\n");
  fprintf(fp, "# each option is allowed to be called only ONCE in a certain section.\n");
  fprintf(fp, "# These options are:\n");
  fprintf(fp, "# (1) eigen-vector (\"Modify Selection(s)->Eigenvector set\" on GLM interface);\n"); 
  fprintf(fp, "# This option does not need any argument:\n");
  fprintf(fp, "# option eigen-vector\n");
  fprintf(fp, "# (2) fir (\"Modify Selection(s)->Finite impulse response\" on GLM interface);\n");
  fprintf(fp, "# This option needs an argument as # of TR:\n");
  fprintf(fp, "# option fir <# of TR>\n");
  fprintf(fp, "# (3) fourier-set (\"Modify Selection(s)->Fourier set\" on GLM interface);\n");
  fprintf(fp, "# This option does not need any argument.\n");
  fprintf(fp, "# Four tags are required to set up Fourier set parameters:\n");
  fprintf(fp, "# (a) fs-period expects a positive integer an period to model\n");
  fprintf(fp, "# (b) fs-harmonics expects a positive integer as the number of harmonics\n");
  fprintf(fp, "# (c) fs-zero-freq expects y/yes or n/no to decide wether frequency zero should be added or not\n");
  fprintf(fp, "# (d) fs-alter-cov expects y/yes or n/no to decide whether covariate will be altered or not\n");
  fprintf(fp, "# A valid fourier-set option will look like:\n");
  fprintf(fp, "option fourier-set\n");  
  fprintf(fp, "  fs-period 5\n");
  fprintf(fp, "  fs-harmonics 3\n");
  fprintf(fp, "  fs-zero-freq y\n");
  fprintf(fp, "  fs-alter-cov n\n");

  fprintf(fp, "# Two options generate a new covariate based on selected one.\n");
  fprintf(fp, "# They are ONLY allowed in \"newcov cp\" section and each option\n");
  fprintf(fp, "# is allowed to be called only ONCE in the section. These options are:\n");
  fprintf(fp, "# (1) exponential (\"Modify Selection(s)->Exponential\" on GLM interface);\n");
  fprintf(fp, "# exponential option needs an argument as exponential value:\n");
  fprintf(fp, "option exponential 2.3\n");
  fprintf(fp, "# (2) derivative (\"Modify Selection(s)->Derivative\" on GLM interface);\n");
  fprintf(fp, "# derivative needs a positive integer argument as the number of derivative(s):\n");
  fprintf(fp, "option derivative 2\n\n");

  fprintf(fp, "# \"end\" tag closes previous section\n");

  fprintf(fp, "# \"newcov\" section creates new covariate(s)\n");
  fprintf(fp, "# Actions that are supported in this section include:\n");
  fprintf(fp, "# single, intercept, trial-effect, diagonal, contrast, var-trialfx\n");
  fprintf(fp, "# scan-effect, global-signal, move-params, spike, txt-file, cp\n\n");

  fprintf(fp, "# (1) \"newcov single <ref filename>\" adds single covariate based on ref file;\n");
  fprintf(fp, "# It needs a string argument as ref filename.\n"); 
  fprintf(fp, "# Default covariate name is \"undefined\"\n");
  fprintf(fp, "# NOTE: if cov-name is available in this section, this name will be overwritten\n");

  fprintf(fp, "newcov single /my_dir/foo.ref\n");
  fprintf(fp, "  # Default type is I for single covariate, so the next line is optional\n");
  fprintf(fp, "  type I\n");
  fprintf(fp, "  # Name it foo instead of undefined\n");
  fprintf(fp, "  cov-name foo\n"); 

  fprintf(fp, "  # do mean-center and convolve on it\n");
  fprintf(fp, "  option mean-center\n");
  fprintf(fp, "  option convolve <kernel_file_path> <kernel_TR> <optional_tag>\n");
  fprintf(fp, "# Close the section by \"end\"\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (2) \"newcov intercept\" adds intercept covariate\n");
  fprintf(fp, "# It takes an optional argument as covariate name\n");
  fprintf(fp, "# default name: Intercept, default type: K\n");
  fprintf(fp, "# cov-name will overwrite name\n");
  fprintf(fp, "newcov intercept\n");
  fprintf(fp, "# This line sets type to \"NoInterest\"\n");
  fprintf(fp, "type N\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (3) \"newcov diagonal\" adds diagonal set\n");
  fprintf(fp, "# It doesn't need any arguments\n");
  fprintf(fp, "# default name: condition key name, default type: I\n");
  fprintf(fp, "newcov diagonal\n");
  fprintf(fp, "# I want to set the type to be NoInterest instead\n");
  fprintf(fp, "  type N\n");
  fprintf(fp, "  # \"scale\" tag sets \"scale by trial count\" option in\n");
  fprintf(fp, "  # \"newcov diagonal\" and \"newcov contrast\" sections.\n");
  fprintf(fp, "  # So it is allowed ONLY in these two sections.\n");
  fprintf(fp, "  # Set \"scale by trial count\" to be yes (default is n/no)\n");
  fprintf(fp, "  scale y\n");
  fprintf(fp, "  # \"center-norm\" tag sets \"center and normalize\" option in\n");
  fprintf(fp, "  # \"newcov diagonal\" and \"newcov contrast\" sections.\n");
  fprintf(fp, "  # So it is allowed ONLY in these two sections.\n");
  fprintf(fp, "  # Set \"center and normalize\" to be No (default is y/yes)\n");
  fprintf(fp, "  center-norm n\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (4) \"newcov contrast\" adds contrast covaraite(s)\n");
  fprintf(fp, "# It doesn't need any arguments\n");
  fprintf(fp, "# default name: \n");
  fprintf(fp, "# <numberic value #1>*<condition key #1> + <numberic value #2>*<condition key #2> + ...\n");
  fprintf(fp, "# default type: I\n");
  fprintf(fp, "newcov contrast\n");  
  fprintf(fp, "  # matrix-row is a tag allowed ONLY in \"newcov contrast\" section.\n");
  fprintf(fp, "  # it defines a certain row of contrast matrix\n");
  fprintf(fp, "  # The number of arguments of matrix-row must be equal to the number of condition keys\n");
  fprintf(fp, "  matrix-row 3 5\n");
  fprintf(fp, "  matrix-row 1.3 4.77\n");
  fprintf(fp, "  # Do you want \"scale by trial count\" option? The default is y/yes\n"); 
  fprintf(fp, "  scale y\n");
  fprintf(fp, "  # Do you want \"center and normalize\" option? The default is y/yes\n"); 
  fprintf(fp, "  center-norm n\n");
  fprintf(fp, "  # put them into a group called contrast01\n");
  fprintf(fp, "  group contrast01\n");
  fprintf(fp, "  # Give them some easy-to-remember names\n");
  fprintf(fp, "  cov-name xxx yyy\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (5) \"newcov trial-effect <trial_length>\" adds trial effect covaraite(s)\n");
  fprintf(fp, "# <trial_length> is the length of trial effect (unit: second)\n");
  fprintf(fp, "# default type: I; default name: trialfx-<index>\n");
  fprintf(fp, "# e.g. the covariates created in this section will be called:\n");
  fprintf(fp, "# trialfx-1, trialfx-2, ...\n");
  fprintf(fp, "newcov trial-effect 40\n");
  fprintf(fp, "  # I want to shift 3 seconds (3000ms) on each new trial effect covariate\n");
  fprintf(fp, "  option time-shift 3000\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (6) \"newcov var-trialfx <trial filename>\" adds var length trial effect covaraite(s)\n");
  fprintf(fp, "# <trial filename> is the file that includes var length\n");
  fprintf(fp, "# default type: I; default name: var-trialfx-<index>\n");
  fprintf(fp, "newcov var-trialfx /home/foo/trial.ref\n");
  fprintf(fp, "  type n\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (7) \"newcov scan-effect\" adds scan effect. No arguments needed\n");
  fprintf(fp, "# default type: N\n; default name: scanfx-<index>");
  fprintf(fp, "newcov scan-effect\n");
  fprintf(fp, "scan-length xxx yyy\n");
  fprintf(fp, "end\n");
  fprintf(fp, "# scan-length line is required if no tes files are defined earlier\n\n");

  fprintf(fp, "# (8) \"newcov global-signal\" adds global signal covariate. No arguments needed\n");
  fprintf(fp, "# default type: N; default name: global-signal-<index>\n");
  fprintf(fp, "newcov global-signal\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (9) \"newcov move-params\" adds movement parameter covariates. No arguments needed\n");
  fprintf(fp, "# default type: N; default name: move-params-<scan index>-X/Y/Z/pitch/roll/yaw\n");
  fprintf(fp, "newcov move-params\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (10) \"newcov spike <spike position>\" adds spike covariates.\n");
  fprintf(fp, "# It needs an array of integers as argument to define the spike position in unit of TR\n");
  fprintf(fp, "# default type: N; default name: spike-<spike position>\n");
  fprintf(fp, "# Note that by default spike covariates are always mean-center'ed\n");
  fprintf(fp, "newcov spike \"10, 20, 67-70\"\n");
  fprintf(fp, "type I\n");
  fprintf(fp, "end\n");
  fprintf(fp, " # Or:\n");
  fprintf(fp, "newcov spike\n");
  fprintf(fp, "absolute \"10, 20, 67-70\"\n");
  fprintf(fp, "type I\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# \"newcov spike\" section can also create spikes whose positions are relative\n"); 
  fprintf(fp, "# to a certain real or fake tes file, for example:\n");
  fprintf(fp, "newcov spike\n");
  fprintf(fp, "absolute 10-12\n");
  fprintf(fp, "relative foo1 \"2,5\"\n"); 
  fprintf(fp, "end\n");
  fprintf(fp, "# The \"absolute\" line creates spike covariates at absolute position of 10, 11 and 12;\n");
  fprintf(fp, "# the \"relative\" line creates 2 spike covariates relative to covariate \"foo1\"\n");
  fprintf(fp, "# \"relative\" accepts both tes file name and tes file index.\n");
  fprintf(fp, "# If \"foo1\" has index of 2, (the 3rd tes file defined), the section above is equivalent to:\n");
  fprintf(fp, "newcov spike\n");
  fprintf(fp, "absolute 10-12\n");
  fprintf(fp, "relative 2 \"2,5\"\n"); 
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (11) \"newcov txt-file <txt filename>\" reads an input txt file and\n");
  fprintf(fp, "# loads each column as a seperate covariate.\n");
  fprintf(fp, "# The argument is txt filename\n");
  fprintf(fp, "# default type: N, default name: txtVar-<column #>\n");
  fprintf(fp, "newcov txt-file /tmp/foo.txt\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# (12) \"newcov cp <covariate name>\" duplicates a covariate that already exists.\n");
  fprintf(fp, "# <covariate_name> is name of the covariate that will be duplicated\n");
  fprintf(fp, "# Note that this section supports two exclusive options:\n");
  fprintf(fp, "# derivative and exponential\n");
  fprintf(fp, "newcov cp <covariate_name>\n");
  fprintf(fp, "  option derivative 2\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# \"modcov <covariate name>\" selects a certain covariate by its name and modify\n"); 
  fprintf(fp, "its properties such as name, group, type, or the vector.\n");
  fprintf(fp, "# Note again that it only supports the options that do NOT create new covariate(s).\n");
  fprintf(fp, "modcov <covariate name>\n");
  fprintf(fp, "cov-name <new covariate name>\n");
  fprintf(fp, "group <new group name>\n");
  fprintf(fp, "type k\n");
  fprintf(fp, "option convert-delta\n");
  fprintf(fp, "option ...\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# \"modcov+ <covariate name>\" is similar to modcov,\n");
  fprintf(fp, "# but it also supports three exclusive options:\n");
  fprintf(fp, "# eigen-vector, fir, fourier-set\n");
  fprintf(fp, "modcov+ <covariate name>\n");
  fprintf(fp, "type n\n");
  fprintf(fp, "  option eigen-vector\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# \"del-cov <covariate name>\" removes a certain covariate\n");
  fprintf(fp, "# <covariate name> is the covariate's FULL name\n");
  fprintf(fp, "del-cov group1->foo\n\n");

  fprintf(fp, "# \"del-all-cov\" removes all covariates\n");
  fprintf(fp, "del-all-cov\n\n");

  fprintf(fp, "# \"openG <G matrix file>\" loads a G matrix file that already exists.\n");
  fprintf(fp, "# If a preG file (upsampled version of G file) with same header coexists\n");
  fprintf(fp, "# in the same directory, the preG file will be loaded instead.\n");
  fprintf(fp, "# When it is loaded, the G/preG file's header is always read first to get TR,\n");
  fprintf(fp, "# of images and condition function. If these parameters in the header are\n");
  fprintf(fp, "# different from previous definition, the latter will be overwritten.\n");
  fprintf(fp, "openG foo.G\n\n");
  
  fprintf(fp, "# chkeff section is designed for efficiency check. It needs five parameters totally:\n");
  fprintf(fp, "# (1) base covariate name; (2) downsample option; (3) filter filename;\n");
  fprintf(fp, "# (4) type of covariates that will be included for efficiency check; (5) cutoff value\n");
  fprintf(fp, "# chkeff is followed by the base covariate's FULL name\n");
  fprintf(fp, "# Base covariate type must match eff-type defined below\n");
  fprintf(fp, "# chkeff myGroup->myName\n");
  fprintf(fp, "  # downsample has three options:\n");
  fprintf(fp, "  # no (no downsample at all),\n");
  fprintf(fp, "  # before (downsample before convolution),\n");
  fprintf(fp, "  # after (downsample after convolution)\n");
  fprintf(fp, "  downsample no/before/after\n");
  fprintf(fp, "  # filter is followed by the path of filter file for efficiency check\n");
  fprintf(fp, "  filter /mypath/foo.ref\n");
  fprintf(fp, "  # eff-type is the covariate type that will be included in efficiency check\n");
  fprintf(fp, "  # three options are acceptable:\n");
  fprintf(fp, "  # A/a (all covariates), I/i (Interest), N/n (NoInterest)\n");
  fprintf(fp, "  # Note that N/n include both NoInterest and KeepNoInterest\n");
  fprintf(fp, "  eff-type A\n");
  fprintf(fp, "  # eff-cutoff is followed by a numeric value.\n");
  fprintf(fp, "  # covariate whose relative efficiency is less than this value wil be deleted\n");
  fprintf(fp, "  eff-cutoff 0.02\n");
  fprintf(fp, "# end is required at the end of chkeff section\n");
  fprintf(fp, "end\n\n");

  fprintf(fp, "# Create another G matrix\n");
  fprintf(fp, "gsession /tmp/bar\n");
  fprintf(fp, " ... ... \n\n");

  fprintf(fp, "# include <filename> line is equivalent to adding the content of\n");
  fprintf(fp, "# <filename> at the position of this line\n");
  fprintf(fp, "include /data/foo.gds\n\n");

}
