#!/bin/bash

# for bin in `ls /usr/local/VoxBo/bin` ; do
#   echo $bin " > " $bin.out >> makehelp.sh ;
# done

analyzeinfo  >  analyzeinfo.out
calcgs  >  calcgs.out
calcps  >  calcps.out
comptraces  >  comptraces.out
cub2pngs  >  cub2pngs.out
dcmsplit > dcmsplit.out
dicominfo  >  dicominfo.out
# display_dataset  >  display_dataset.out
# extractmask  >  extractmask.out
# ffinfo  >  ffinfo.out
fillmask  >  fillmask.out
gcheck >  gcheck.out
gds  >  gds.out
gdw -h  >  gdw.out
# getdata  >  getdata.out
glm -h >  glm.out
glminfo  >  glminfo.out
imginfo > imginfo.out
# idlfree  >  idlfree.out
# makeLittleEndian.sh  >  makeLittleEndian.sh.out
makematk  >  makematk.out
makematkg  >  makematkg.out
makevlsm > makevlsm.out
merge3d  >  merge3d.out
# minprep.sh  >  minprep.sh.out
# newprep.sh  >  newprep.sh.out
niftiinfo > niftiinfo.out
norm  >  norm.out
# oldprep.sh  >  oldprep.sh.out
permstep  >  permstep.out
# plotfdr > plotfdr.out
# print_dataset  >  print_dataset.out
# putdata  >  putdata.out
realign  >  realign.out
resample  >  resample.out
setorigin  >  setorigin.out
sliceacq  >  sliceacq.out
sortmvpm  >  sortmvpm.out
spm8_coreg > spm8_coreg.out
spm8_realign > spm8_realign.out
spm8_reslice > spm8_reslice.out
spm8_segment > spm8_segment.out
spm8_warp > spm8_warp.out
submit_sequence  >  submit_sequence.out
sumrfx  >  sumrfx.out
tes2cub  >  tes2cub.out
tesjoin  >  tesjoin.out
tesplit  >  tesplit.out
txt2num  >  txt2num.out
vb2cub  >  vb2cub.out
vb2img  >  vb2img.out
vb2imgs  >  vb2imgs.out
vb2tes  >  vb2tes.out
vb2vmp  >  vb2vmp.out
vbbatch  >  vbbatch.out
vbcfx  >  vbcfx.out
vbcmp  >  vbcmp.out
# vbconf  >  vbconf.out
vbconv  >  vbconv.out
vbdumpstats  >  vbdumpstats.out
vbfdr > vbfdr.out
vbfilter  >  vbfilter.out
vbfit  >  vbfit.out
vbhdr  >  vbhdr.out
vbi  >  vbi.out
vbim  >  vbim.out
vbimagemunge  >  vbimagemunge.out
vbinterpolate  >  vbinterpolate.out
vblock >  vblock.out
vbmakefilter  >  vbmakefilter.out
vbmakeglm  >  vbmakeglm.out
# vbmakeprep  >  vbmakeprep.out
vbmakeregress  >  vbmakeregress.out
vbmakeresid  >  vbmakeresid.out
vbmaskcompare  >  vbmaskcompare.out
vbmaskinfo  >  vbmaskinfo.out
vbmaskmunge  >  vbmaskmunge.out
vbmerge4d  >  vbmerge4d.out
vbmm2  >  vbmm2.out
vbmunge  >  vbmunge.out
vborient  >  vborient.out
vboverlap  >  vboverlap.out
vbpermgen -h  >  vbpermgen.out
vbperminfo  >  vbperminfo.out
vbpermmat  >  vbpermmat.out
vbpermvec  >  vbpermvec.out
# vbprefs  >  vbprefs.out
vbprep  >  vbprep.out
# vbqa  >  vbqa.out
vbregion  >  vbregion.out
vbregress  >  vbregress.out
vbrename  >  vbrename.out
vbrender  >  vbrender.out
# vbri
vbscoregen > vbscoregen.out
# vbscale
# vbscoregen
vbse  >  vbse.out
vbshift  >  vbshift.out
vbsim  >  vbsim.out
vbsmooth  >  vbsmooth.out
# vbsrvd  >  vbsrvd.out
vbstatmap  >  vbstatmap.out
vbtcalc -h  >  vbtcalc.out
vbthresh  >  vbthresh.out
vbtmap > vbtmap.out
vbtool > vbtool.out
vbvec2hdr  >  vbvec2hdr.out
vbview2 -h  >  vbview2.out
vbvolregress  >  vbvolregress.out
vbxts  >  vbxts.out
vecsplit  >  vecsplit.out
vecview  >  vecview.out
voxbo -h  >  voxbo.out
voxq  >  voxq.out
