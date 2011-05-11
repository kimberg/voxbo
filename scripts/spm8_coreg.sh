#!/bin/bash
# run SPM8 coregistration

# see spm8/config/spm_run_coreg_estwrite.m

if [[ "$MATLABPATH" =~ spm8 && -z "$SPM8PATH" ]] ; then SPM8PATH="$MATLABPATH"; fi
if [[ -z "$SPM8PATH" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8PATH" && -d /usr/local/spm8 ]] ; then SPM8PATH=/usr/local/spm8; fi
if [[ -z "$SPM8PATH" && -d /usr/lib/spm8 ]] ; then SPM8PATH=/usr/share/matlab/site/m/spm8; fi
if [[ -z "$SPM8CMD" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8CMD" ]] ; then SPM8CMD='matlab -nodesktop -nosplash -nojvm'; fi

# defaults
MATLABPATH=$SPM8PATH
SPM8_MATLAB_CMD=$SPM8CMD  # formerly "/usr/local/bin/matlab2009b -nodesktop -nosplash -nojvm"
INFILE='3dfile.nii'
REF='ref.nii'

printhelp() {
  echo "  "
  echo "VoxBo spm8_coreg script"
  echo "summary: script for running SPM8 coregistration"
  echo "usage:"
  echo "  spm8_segment <flags>"
  echo "the following flags are honored:"
  echo "  -r <file>   filename of reference volume"
  echo "  -i <file>   filename of 3D volume to coregister"
  echo "  -h          print this help"
  echo "  "
}

if [[ $# -lt 2 ]] ; then
  printhelp;
  exit;
fi

while getopts hr:i: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
    i) INFILE=$OPTARG ;;
    r) REF=$OPTARG ;;
    \\?) echo "argh!" ;;
  esac
done

DIR=$(dirname $INFILE)
FNAME=$(basename $INFILE)

echo "+---------------------------------+"
echo "| SPM8 coregistration bash script |"
echo "+---------------------------------+"
echo " infile: " $INFILE
echo "    ref: " $REF

$SPM8_MATLAB_CMD << EOF
spm('Defaults','fMRI')
global defaults;
[v1,v2]=spm('Ver')
v3=spm('MLver')
fprintf('SPM version %s/%s, MATLAB version %s\n',v1,v2,v3)

cflags1.cost_fun = 'nmi';
cflags1.sep      = [4 2];
cflags1.tol      = [0.02 0.02 0.02 0.001 0.001 0.001 0.01 0.01 0.01 0.001 0.001 0.001];
cflags1.fwhm     = [7 7];

cflags2.prefix     = 'r';
cflags2.interp      = 1;
cflags2.wrap        = [0 0 0];
cflags2.mask        = 0;
cflags2.mean = 0;
cflags2.which=1;

% spm_coreg returns a rigid body rotation
x=spm_coreg('$REF','$INFILE',cflags1)

newspace=inv(spm_matrix(x))*spm_get_space('$INFILE')
spm_get_space('$INFILE',newspace)
P=strvcat('$REF','$INFILE')
spm_reslice(P,cflags2)
EOF

echo

if [[ ! -f $DIR/r$FNAME ]] ; then exit 1 ; fi

if [[ -n $OUTFILE ]] ; then
  mv $DIR/w$FNAME $DIR/$OUTFILE
  mv $DIR/w${FNAME%.*}.mat ${OUTFILE%.*}.mat
fi

exit 0

