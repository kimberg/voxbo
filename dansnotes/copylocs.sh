#!/bin/bash2

# copylocs.sh - This script copies the appropriate localizers to a
# subject's Anatomy directory, creating the latter if appropriate.  It
# really only creates two files: Norm.cub and EPI.cub.  See
# preplocs.sh for some more details.

TDIR=/usr/local/VoxBo/elements/templates
ORIGIN_X="none"
ORIGIN_Y="none"
ORIGIN_Z="none"

if [ "$VOXBIN" == "" ] ; then
  VOXBIN=`vbconf bindir`
fi

if [ "$VOXBIN" == "" ] ; then
  echo "Couldn't find VoxBo binary directory."
  exit 110
fi

ADIR=$1/Anatomy
if [ "$ADIR" == "" ] ; then exit 102 ; fi
if [ ! -e "$ADIR" ] ; then mkdir $ADIR ; fi
# FIXME following line should also check that adir/locs doesn't exist
if [ -d $1/locs ] ; then mv $1/locs $ADIR ; fi
LDIR=`ls -1d $ADIR/locs/[0-9][0-9][0-9][0-9][0-9]`
if [ "$LDIR" == "" ] ; then exit 101 ; fi
SLIST=`ls $LDIR`
if [ "$SLIST" == "" ] ; then exit 103 ; fi
for series in $SLIST ; do
  STYPE=`$VOXBIN/scantype $LDIR/$series`
  if [ "$STYPE" == "fgre3d-axial" ] ; then ANAT=$series ; fi
  if [ "$STYPE" == "3dgrass-axial" ] ; then ANAT=$series ; fi
  if [ "$STYPE" == "memp-axial" ] ; then ANAT=$series ; fi
  if [ "$STYPE" == "epiperf-axial" ] ; then FUNC=$series ; fi
  if [ "$STYPE" == "fairest_raw61-axial" ] ; then FUNC=$series ; fi
  if [ "$STYPE" == "fairest_raw6-axial" ] ; then FUNC=$series ; fi
  if [ "$STYPE" == "fmriepi-axial" ] ; then FUNC=$series ; fi
  if [ "$STYPE" == "fmrigate-axial" ] ; then FUNC=$series ; fi
  if [ "$STYPE" == "memp-sagittal" ] ; then SAG=$series ; fi
  if [ "$STYPE" == "fgre-sagittal" ] ; then SAG=$series ; fi
done

# honor parameters that set the series or origin
for arg in $* ; do
  if [[ ${arg:0:5} == "ANAT=" && ${arg:5:999} != "none" ]] ; then ANAT=${arg:5:999} ; fi
  if [[ ${arg:0:4} == "SAG=" && ${arg:4:999} != "none" ]] ; then SAG=${arg:4:999} ; fi
  if [[ ${arg:0:5} == "FUNC=" && ${arg:5:999} != "none" ]] ; then FUNC=${arg:5:999} ; fi
  if [[ ${arg:0:9} == "ORIGIN_X=" && ${arg:9:999} != "none" ]] ; then ORIGIN_X=${arg:9:999} ; fi
  if [[ ${arg:0:9} == "ORIGIN_Y=" && ${arg:9:999} != "none" ]] ; then ORIGIN_Y=${arg:9:999} ; fi
  if [[ ${arg:0:9} == "ORIGIN_Z=" && ${arg:9:999} != "none" ]] ; then ORIGIN_Z=${arg:9:999} ; fi
done

echo "Localizer directory:" $LDIR
echo "Anatomy directory:" $ADIR
echo "Sagittal series:" $SAG
echo "Anatomical series:" $ANAT
echo "Functional series:" $FUNC

if [ "$ANAT" == "none-none" ] || [ "$FUNC" == "none-none" ] ; then
  echo "bad directory " $1
  exit 104 ;
fi
if [ "$ANAT" == "" ] || [ "$FUNC" == "" ] ; then
  echo "bad locs " $1
  exit 105 ;
fi

# extract the anatomical for normalization, shifted if needed
# old line before recentering hack (dyk): $VOXBIN/vb2cub $LDIR/$ANAT $ADIR/Norm.cub
$VOXBIN/recenter $LDIR/$ANAT $ADIR/Norm.cub -rc $LDIR/$FUNC
if (($? > 0)) ; then exit 101 ; fi

# if we have an origin, use it instead
if [[ "$ORIGIN_X" != "" && "$ORIGIN_X" != "none" ]] ; then
  echo "Setting origin to: " $ORIGIN_X $ORIGIN_Y $ORIGIN_Z
  $VOXBIN/setorigin $ADIR/Norm.cub $ORIGIN_X $ORIGIN_Y $ORIGIN_Z
  if (($? > 0)) ; then exit 103 ; fi
else
  echo "Guessing origin."
  $VOXBIN/setorigin -g $ADIR/Norm.cub
  if (($? > 0)) ; then exit 102 ; fi
fi

# extract and crop the sample EPI for origin fun
$VOXBIN/vb2cub $LDIR/$FUNC $ADIR/EPI.cub
if (($? > 0)) ; then exit 104 ; fi
$VOXBIN/resample $ADIR/EPI.cub $ADIR/EPI.cub -xx 12 1 40
if (($? > 0)) ; then exit 105 ; fi

# create display locs
if [ "$SAG" != "" ] ; then
  $VOXBIN/vb2cub $LDIR/$SAG $ADIR/Sagittals.cub
  if (($? > 0)) ; then exit 106 ; fi
fi

echo "copylocs.sh: Done with directory '$1'"
