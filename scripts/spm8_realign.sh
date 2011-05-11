#!/bin/bash
# realign one or more time series volumes to a specific reference
# image using SPM8

if [[ "$MATLABPATH" =~ spm8 && -z "$SPM8PATH" ]] ; then SPM8PATH="$MATLABPATH"; fi
if [[ -z "$SPM8PATH" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8PATH" && -d /usr/local/spm8 ]] ; then SPM8PATH=/usr/local/spm8; fi
if [[ -z "$SPM8PATH" && -d /usr/lib/spm8 ]] ; then SPM8PATH=/usr/share/matlab/site/m/spm8; fi
if [[ -z "$SPM8CMD" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8CMD" ]] ; then SPM8CMD='matlab -nodesktop -nosplash -nojvm'; fi

# some defaults
MATLABPATH=$SPM8PATH
SPM8_MATLAB_CMD=$SPM8CMD  # formerly "/usr/local/bin/matlab2009b -nodesktop -nosplash -nojvm"
REF='ref.nii'
INFILE='data.nii'
OUTFILE=

printhelp() {
  echo "  "
  echo "VoxBo spm8_realign script"
  echo "summary: script for realigning using SPM8"
  echo "usage:"
  echo "  spm8_realign <flags>"
  echo "the following flags are honored:"
  echo "  -i <name>   file to realign"
  echo "  -r <name>   3D ref volume"
  echo "  -o <name>   set output file name (default: r is prepended)"
  echo "  -h          print this help"
  echo "  "
}

if [[ $# -lt 3 ]] ; then
  printhelp;
  exit;
fi

while getopts ho:i:r: opt; do
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

echo "+------------------------------+"
echo "| SPM8 realignment bash script |"
echo "+------------------------------+"
echo "    ref: " $REF
if [[ -n $OUTFILE ]] ; then
  echo "outfile: " $OUTFILE
else
  echo "outfile: " $DIR/r$FNAME
fi

$SPM8_MATLAB_CMD << EOF
spm('Defaults','fMRI')
global defaults;
[v1,v2]=spm('Ver')
v3=spm('MLver')
fprintf('SPM version %s/%s, MATLAB version %s\n',v1,v2,v3)

rflags1.quality = 1;
rflags1.weight = {''};
rflags1.interp = 2;
rflags1.wrap   = [0 0 0];
rflags1.sep    = 4;
rflags1.fwhm   = 5;
rflags1.rtm    = 0;
rflags2.prefix   = 'r';
rflags2.mask      = 1;
rflags2.interp    = 4;
rflags2.wrap      = [0 0 0];
rflags2.which     = 2;

refplus=strvcat('$REF','$INFILE');
spm_realign(refplus,rflags1)
spm_reslice('$INFILE',rflags2)
exit
EOF

echo

if [[ ! -f $DIR/r$FNAME ]] ; then exit 1 ; fi

if [[ -n $OUTFILE ]] ; then
#  DIR2=`dirname $OUTFILE`
  mv $DIR/r$FNAME $OUTFILE
  mv $DIR/r${FNAME%.*}.mat ${OUTFILE%.*}.mat
fi

exit 0
