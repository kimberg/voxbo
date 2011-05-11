#!/bin/bash
# script to do perfusion processing

# defaults
#MATLABPATH=/usr/local/spm8
#SPM8_MATLAB_CMD="matlab2009b -nodesktop"
MPRAGE=mprage.nii
PERF=perf.nii
WMASK=wmask.nii
FSLOUTPUTTYPE=NIFTI    # for spm-compatibility

# FIXME test return codes at every step


# reset orientation info to sane defaults
vbim $MPRAGE -setspace -write $MPRAGE
vbim $PERF -setspace -write4d $PERF
vbim $WMASK -setspace -write $WMASK


# remove existing files from previous runs that could confuse things
rm -f *perf1* rperf* mperf* *.mat meanperf* [wm]mprage.nii *perfmap* *~ *.ps rp_*.txt

# coreg perf to mprage using FLIRT
#flirt -cost normmi -in $PERF -ref $MPRAGE -out cperf.nii -omat perf2mprage.mat
# coreg perf to mprage using resample
#resample perf.tes cperf.tes -ref mprage.cub

# coreg perf to mprage using spm
vbim perf.nii -include 0 -write perf1.nii
spm8_coreg -r $MPRAGE -i perf1.nii
vbim perf.nii -setspace perf1.nii -write4d perf.nii

# for convenience, extract perf1
#vbim cperf.nii -include 0 -setspace -write perf1.nii
#vbim $PERF -setspace -write4d $PERF

# segment first volume of PERF for the sole purpose of bias correction
# (producing mperf.nii), remove extraneous normalized version
# (wperf.nii).  we do this first so that we aren't going to mess with
# PERF's coregistration.
spm8_segment -i perf.nii,1 -o tmp.nii -p tmp.mat
#rm -f wperf.nii tmp.mat tmp.nii
# get perf denominator
#denom=`calcperf -t 4000 -m  $WMASK perf1.nii`
denom=`calcperf -t 4000 -m  $WMASK mperf.nii`
echo "DENOM: " $denom
#rm mperf.nii

# reset NIfTI headers on input files to sane defaults
vbim $MPRAGE -setspace -write $MPRAGE
#vbim $PERF -setspace -write4d $PERF
vbim $WMASK -setspace -write $WMASK

# extract the first volume as a reference for realignment
# don't need to do this, we can just use "foo.nii,1" within spm
# vbim $PERF -include 0 -write perf1.nii

# coregister first perf volume to mprage
#spm8_coreg -r $MPRAGE -i perf1.nii

# realign and reslice the single perf volume
spm8_realign -i perf.nii -r perf1.nii
#flirt -in cperf.nii -ref perf1.nii -out rperf.nii


# get the perf data, reduce it to a perf map
vbim rperf.nii -convert double  -oddeven -average -div $denom -write perfmap.nii
# the following may be necessary, but shouldn't be, since we resliced
vbim perfmap.nii -setspace perf1.nii -write perfmap.nii

# normalize (segment) mprage
spm8_segment -i $MPRAGE -p sn_seg.mat

# normalize perf map
spm8_warp -i perfmap.nii -p sn_seg.mat -b t1
