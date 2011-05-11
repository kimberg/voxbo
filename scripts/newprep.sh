#!/bin/bash

# newprep.sh - This script assumes you pass it one or more directory
# names that themselves contain Anatomy directories.  Each such
# directory must contain an Anatomical.cub file and a Functional.cub
# file, containing the high-res anatomical data in the former and a
# sample of your functional data in the latter.  Each file must
# contain an absolute corner position and voxel sizes.

TDIR=/usr/local/VoxBo/elements/templates
PWD=`pwd`
BPWD=`basename $PWD`

if (( $# > 0)) ; then
  ADIRS=$*
elif [ "$BPWD" == "Anatomy" ] ; then
  ADIRS="."
elif [ -d "Anatomy" ] ; then
  ADIRS="Anatomy"
else 
  echo 'argh'
  exit 255
fi

for DIR in $ADIRS ; do

  if [ -d "$DIR/Anatomy" ] ; then ADIR=$DIR/Anatomy
  else ADIR=$DIR ; fi

# Copy the estimated origin from Anatomical.cub to Functional.cub and back.  This ensures
# that the origin on Norm will be on an EPI voxel boundary

  setorigin -m $ADIR/Anatomical.cub $ADIR/Functional.cub
  if (($? > 0)) ; then exit 101 ; fi
  setorigin -m $ADIR/Functional.cub $ADIR/Anatomical.cub
  if (($? > 0)) ; then exit 101 ; fi

# Now calculate norm params and apply as a test to Anatomical.cub

  norm -calc -r $TDIR/single_subj_T1.cub -p $ADIR/NormParams.ref $ADIR/Anatomical.cub
  if (($? > 0)) ; then exit 101 ; fi
  norm -apply -p $ADIR/NormParams.ref -o $ADIR/nAnatomical.cub $ADIR/Anatomical.cub
  if (($? > 0)) ; then exit 101 ; fi

# Finally, the more mundane stuff...

# create a Display.cub that's in the same space as the functional images

  resample $ADIR/Anatomical.cub $ADIR/Display.cub -ra $ADIR/Functional.cub
  if (($? > 0)) ; then exit 107 ; fi

# normalize the display locs, just for kicks

  norm -apply -p $ADIR/NormParams.ref -z $ADIR/Functional.cub -o $ADIR/nDisplay.cub $ADIR/Anatomical.cub
  if (($? > 0)) ; then exit 109 ; fi

# create a coverage version called cDisplay.cub

  norm -apply -p $ADIR/NormParams.ref -o $ADIR/cDisplay.cub $ADIR/Display.cub
  if (($? > 0)) ; then exit 109 ; fi

# normalize the EPIs, just for kicks

  norm -apply -p $ADIR/NormParams.ref -o $ADIR/nFunctional.cub $ADIR/Functional.cub
  if (($? > 0)) ; then exit 109 ; fi

  echo "newprep.sh: Done with directory '$ADIR'"

done
