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
OUTFILE=
TDIR="/usr/local/spm8/tpm"

printhelp() {
  echo "  "
  echo "VoxBo spm8_segment script"
  echo "summary: script for running SPM8 unified segmentation"
  echo "usage:"
  echo "  spm8_segment <flags>"
  echo "the following flags are honored:"
  echo "  -p <file>   parameter file to write (default: seg.mat)"
  echo "  -o <file>   normalized file to write (default: wmprage.nii)"
  echo "  -t <dir>    dir containing template (default: /usr/local/spm8/tpm)"
  echo "  -i <file>   3D volume to segment"
  echo "  -h          print this help"
  echo "notes:"
  echo "  This script uses SPM8's unified segmentation to calculate a warp"
  echo "  from your volume to the template included with SPM8."
  echo "  "
  echo "  Note that the template is segmented, and composed of three files:"
  echo "  gray.nii, white.nii, and csf.nii"
  echo "  "
  echo "  SPM8 handles uncompressed NIfTI files."
  echo "  "
}

if [[ $# -lt 2 ]] ; then
  printhelp;
  exit;
fi

while getopts ho:p:i: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
    i) INFILE=$OPTARG ;;
    o) OUTFILE=$OPTARG ;;
    t) TDIR=$OPTARG ;;
    p) PARAMFILE=$OPTARG ;;
    \\?) echo "argh!" ;;
  esac
done

DIR=`dirname $INFILE`
FNAME=`basename $INFILE`

echo "+---------------------------------------+"
echo "| SPM8 unified segmentation bash script |"
echo "+---------------------------------------+"
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

opts0.tpm=char('$TDIR/grey.nii','$TDIR/white.nii','$TDIR/csf.nii');
opts0.ngaus    = [2 2 2 4];
opts0.warpreg  = 1;
opts0.warpco   = 25;
opts0.biasreg  = 0.0001;
opts0.biasfwhm = 60;
opts0.regtype  = 'mni';
opts0.fudge    = 5;
opts0.samp     = 3;
opts0.msk      = '';
opts1.biascor=1;
opts1.cleanup=0;
% three parts of GM/WM/CSF arg are for: native, modulated, unmodulated
opts1.GM=[0 0 1];   
opts1.WM=[0 0 1];
opts1.CSF=[0 0 1];

P=spm_vol('$INFILE')
res=spm_preproc(P,opts0)
[p,ip]=spm_prep2sn(res)

% save the forward params
fn = fieldnames(p);
for i=1:length(fn),
    eval([fn{i} '= p.' fn{i} ';']);
end;
save('$PARAMFILE',fn{:});

% in case i want the inverse params later
fn = fieldnames(ip);
for i=1:length(fn),
    eval([fn{i} '= ip.' fn{i} ';']);
end;
save('seg_inv_sn.mat',fn{:});

% write the segmented and bias corrected files
spm_preproc_write(p,opts1)

% now warp it
opts3.bb=[Inf];   % means to use template bb
opts3.vox=[1 1 1];
opts3.wrap=[0 0 0];
spm_write_sn('$INFILE',p,opts3)

EOF

echo

if [[ ! -f $DIR/w$FNAME ]] ; then exit 1 ; fi

if [[ -n $OUTFILE ]] ; then
  mv $DIR/w$FNAME $OUTFILE
  mv $DIR/w${FNAME%.*}.mat ${OUTFILE%.*}.mat
fi

exit 0
