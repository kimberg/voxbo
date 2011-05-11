#!/bin/bash
# run SPM8 unified segmentation
# TODO:
#  accept bounding box argument

# see spm_preproc.m
# see spm_preproc_write.m

if [[ "$MATLABPATH" =~ spm8 && -z "$SPM8PATH" ]] ; then SPM8PATH="$MATLABPATH"; fi
if [[ -z "$SPM8PATH" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8PATH" && -d /usr/local/spm8 ]] ; then SPM8PATH=/usr/local/spm8; fi
if [[ -z "$SPM8PATH" && -d /usr/lib/spm8 ]] ; then SPM8PATH=/usr/share/matlab/site/m/spm8; fi
if [[ -z "$SPM8CMD" && -r /etc/voxbo/setpaths.sh ]] ; then . /etc/voxbo/setpaths.sh; fi
if [[ -z "$SPM8CMD" ]] ; then SPM8CMD='matlab -nodesktop -nosplash -nojvm'; fi

# defaults
MATLABPATH=$SPM8PATH
SPM8_MATLAB_CMD=$SPM8CMD  # formerly "/usr/local/bin/matlab2009b -nodesktop -nosplash -nojvm"
PARAMFILE='seg.mat'
INFILE='mprage.nii'
OUTFILE=''
BBNAME="t2"
#BB="[-78,-111,-50;80,77,86]"
#VOX="[1 1 1]"
BB="nan(2,3)"
VOX="nan(1,3)"

printhelp() {
  echo "  "
  echo "VoxBo spm8_warp script"
  echo "summary: script for applying SPM8 norm/segment params to an image"
  echo "usage:"
  echo "  spm8_warp <flags>"
  echo "the following flags are honored:"
  echo "  -p <file>   name for parameter file to be used"
  echo "  -i <file>   filename of volume to warp"
  echo "  -o <file>   filename of output volume"
  echo "  -b <spec>   see below"
  echo "  -h          print this help"
  echo "notes:"
  echo "  This script uses SPM8's unified segmentation to calculate a warp"
  echo "  from your volume to the template included with SPM8."
  echo "  "
  echo "  Note that the template is segmented, and composed of three files:"
  echo "  gray.nii, white.nii, and csf.nii"
  echo "  "
  echo "  the -b argument specifies the voxel sizes and bounding box:"
  echo "    t1 -- template space in 1mm"
  echo "    t2 -- template space in 2mm"
  echo "    t3 -- template space in 3mm"
  echo "    s1 -- SPM bounding box in 1mm"
  echo "    s3 -- SPM bounding box in 3mm"
  echo "  "
  echo "  SPM8 handles uncompressed NIfTI files."
  echo "  "
}

if [[ $# -lt 2 ]] ; then
  printhelp;
  exit;
fi

while getopts hp:i:o:b: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
    i) INFILE=$OPTARG; ;;
    o) OUTFILE=$OPTARG ;;
    b) BBNAME=$OPTARG ;;
    p) PARAMFILE=$OPTARG ;;
    \\?) echo "argh!" ;;
  esac
done

DIR=`dirname $INFILE`
FNAME=`basename $INFILE`

if [[ $BBNAME == "s1" ]] ; then
  # voxbo 1mm 159x189x136
  echo s1
  BB="[-78,-111,-50;80,77,86]"
  VOX="[1 1 1]"
elif [[ $BBNAME == "s3" ]] ; then
  # voxbo 3mm 53x63x46
  echo s3
  BB="[-78,-111,-51;78,75,84]"
  VOX="[3 3 3]"
elif [[ $BBNAME == "t1" ]] ; then
  # SPM2 template 1mm (cf ch2) 
  echo t1
  BB="[-90,-126,-72;90,90,108]"
  VOX="[1 1 1]"
elif [[ $BBNAME == "t2" ]] ; then
  # SPM2 template 2mm (cf ch2)
    echo t2
  BB="[-90,-126,-72;90,90,108]"
  VOX="[2 2 2]"
elif [[ $BBNAME == "t3" ]] ; then
  # SPM2 template 3mm (cf ch2)
    echo t3
  BB="[-90,-126,-72;90,90,108]"
  VOX="[3 3 3]"
else
  echo "[E] invalid BB name"
  exit 22
fi


echo "+------------------------------+"
echo "| SPM8 warp (warp) bash script |"
echo "+------------------------------+"
echo " infile: " $INFILE
if [[ -n $OUTFILE ]] ; then
  echo "outfile: " $OUTFILE
else
  echo "outfile: " $DIR/w$FNAME
fi
echo " params: " $PARAMFILE

$SPM8_MATLAB_CMD << EOF
spm('Defaults','fMRI')
global defaults;
[v1,v2]=spm('Ver')
v3=spm('MLver')
fprintf('SPM version %s/%s, MATLAB version %s\n',v1,v2,v3)

p=load('$PARAMFILE');

opts3.bb=$BB;
opts3.vox=$VOX;
opts3.wrap=[0 0 0];
spm_write_sn('$INFILE',p,opts3)

EOF

echo

if [[ ! -f $DIR/w$FNAME ]] ; then exit 1 ; fi

if [[ -n $OUTFILE ]] ; then
  mv $DIR/w$FNAME $OUTFILE
# scunge from another file???  mv $DIR/w${FNAME%.*}.mat ${OUTFILE%.*}.mat
fi

exit 0
