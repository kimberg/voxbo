#!/bin/bash
# run SPM8 reslice
# see spm8/???

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
OUTFILE=

printhelp() {
  echo "  "
  echo "VoxBo spm8_reslice script"
  echo "summary: script for reslicing an image in SPM8"
  echo "usage:"
  echo "  spm8_reslice <flags>"
  echo "the following flags are honored:"
  echo "  -r <file>   filename of reference volume"
  echo "  -i <file>   filename of volume to reslice"
  echo "  -o <file>   filename for output"
  echo "  -h          print this help"
  echo "  "
}

if [[ $# -lt 2 ]] ; then
  printhelp;
  exit;
fi

while getopts hr:i:o: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
    i) INFILE=$OPTARG ;;
    r) REF=$OPTARG ;;
    o) OUTFILE=$OPTARG ;;
    \\?) echo "argh!" ;;
  esac
done

DIR=`dirname $INFILE`
FNAME=`basename $INFILE`

echo "+---------------------------------+"
echo "| SPM8 coregistration bash script |"
echo "+---------------------------------+"
echo " infile: " $INFILE
echo "    ref: " $REF
echo "outfile: " $OUTFILE

$SPM8_MATLAB_CMD << EOF
spm('Defaults','fMRI')
global defaults;
[v1,v2]=spm('Ver')
v3=spm('MLver')
fprintf('SPM version %s/%s, MATLAB version %s\n',v1,v2,v3)

cflags2.prefix     = 'r';
cflags2.interp      = 1;
cflags2.wrap        = [0 0 0];
cflags2.mask        = 0;
cflags2.mean = 0;
cflags2.which=1;

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

