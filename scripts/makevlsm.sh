#!/bin/bash

# some defaults
CMD=vbtmap
DEST=permdir              # destination for permutations
PERMS=0                   # number of permutations
LESIONFILE=lesions.nii.gz # name of the 4D file containing lesion maps
SCOREFILE=scores.ref      # name of the 1D file containing your scores
BATCH=0
TRUE=true.nii.gz

printhelp() {
  echo "  "
  echo "VoxBo makevlsm"
  echo "summary: script for running simple VLSM analyses"
  echo "usage:"
  echo "  makevlsm <flags>"
  echo "the following flags are honored:"
  echo "  -b          submit to voxbo queue"
  echo "  -d <dest>   set destination dir for permutations (default: permdir)"
  echo "  -p <num>    set number of permutations (default: 0)"
  echo "  -o <name>   set filename for unpermuted map (default: true.nii.gz)"
  echo "  -l <file>   set name of 4D lesion map file (default: lesions.nii.gz)"
  echo "  -s <file>   set name of score file (default: scores.ref)"
  echo "  -w          use welch's instead of regular t-test"
  echo "  -z          generate z maps, not t maps"
  echo "  -q <q>      set FDR criterion"
  echo "  -n <num>    set minimum number of patients per voxel (default: 2)"
  echo "  -m <mask>   set inclusion mask (nonzero voxels are included)"
  echo "  -f          flip sign of t map"
  echo "  -x          use chi-squared instead of t-test"
  echo "  -y          apply yates correction to chi-squared"
  echo "notes:"
  echo "  Look at the defaults above.  If you're happy with them, then you can"
  echo "  just run makevlsm with no arguments.  Note that by default makevlsm"
  echo "  uses vbtmap to carry out a t-test.  If you need a chi-squared, use"
  echo "  the -x flag and it will use vbxmap instead."
  echo "  "
  echo "  The -q option causes vbtmap to print an FDR threshold at the terminal."
  echo "  Usually you wouldn't use it along with -b."
  echo "  "
  echo "  The -w option is incompatible with the chi-squared test, and the -y"
  echo "  option is incompatible with the t-test."
  echo "  "
}

if [[ $# -lt 1 ]] ; then
  printhelp;
  exit;
fi

while getopts hbd:p:s:l:n:wzm:fyxgq:o: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
    g) ;;
    b) BATCH=1 ;;
    d) DEST=$OPTARG ;;
    p) PERMS=$OPTARG ;;
    l) LESIONFILE=$OPTARG ;;
    o) TRUE=$OPTARG ;;
    s) SCOREFILE=$OPTARG ;;
    m) FLAGS+="-m $OPTARG " ;;
    w) FLAGS+="-w " ;;
    z) FLAGS+="-z " ;;
    n) FLAGS+="-n $OPTARG " ;;
    f) FLAGS+="-f " ;;
    q) FLAGS+="-q $OPTARG " ;;
    x) CMD=vbxmap ;;
    y) FLAGS+="-y " ;;
    \\?) echo "argh!" ;;
  esac
done

NTES=`vbi -d $LESIONFILE | cut --delimiter=' ' --fields=3 | cut --delimiter='x' --fields=4`
NVEC=`vbi -d $SCOREFILE | cut --delimiter=' ' --fields=3`

if [[ $NTES -ne $NVEC ]] ; then
  echo "[E] makevlsm: the number of volumes does not match the number of scores"
  exit 22
fi


if [[ $PERMS -gt 0 ]] ; then
  vbpermmat perms.mat $PERMS $LESIONFILE data
  mkdir -p $DEST
fi

if [[ BATCH -gt 0 ]] ; then
  vbbatch -f vlsmbatch
  vbbatch -m 500 -a vlsmbatch -c "$CMD $LESIONFILE $SCOREFILE ${TRUE} $FLAGS" x
  vbbatch -m 500 -a vlsmbatch -c "$CMD $LESIONFILE $SCOREFILE $DEST/cube_IND.nii.gz $FLAGS -op perms.mat IND" -d $PERMS 
  vbbatch -m 500 -s vlsmbatch
else
  echo "[I] makevlsm: Building unpermuted volume as ${TRUE}"
  $CMD $LESIONFILE $SCOREFILE ${TRUE} $FLAGS
  for (( IND=0 ; IND < $PERMS ; IND++ ))
  do
    indstr=`printf %05d $IND`
    echo "[I] makevlsm: Building permuted volume $IND as $DEST/cube_${indstr}.nii.gz"
    $CMD $LESIONFILE $SCOREFILE $DEST/cube_${indstr}.nii.gz $FLAGS -op perms.mat $IND
  done
fi
