#!/bin/sh

# anat.nii.gz was created by taking ch2.nii.gz and resampling thusly:
#   resample ch2.nii.gz anat.nii.gz -xx 0 2 90 -yy 0 2 108 -zz 33 10 10
# mask created using
#   resample ch2bet.nii.gz mask.nii.gz -xx 0 2 90 -yy 0 2 108 -zz 33 10 10
#   vbim mask.nii.gz -thresh 0.5 -quantize 1 -write mask.nii.gz

# create data

vbsim -d 30 36 10 20 -n 10 5 10 -o small1.nii.gz -s 786876
vbsim -d 30 36 10 20 -n 10 5 10 -o small2.nii.gz -s 123451
resample ../mask.nii.gz mask.nii.gz -xx 0 3 30 -yy 0 3 36 -zz 0 1 10
vbim small1.nii.gz -mult mask.nii.gz -write4d small1.nii.gz
# no longer masking the second one, the masks should be combined
#vbim small2.nii.gz -mult mask.nii.gz -write4d small2.nii.gz

cat > test.glm<< EOF
pieces 4
audit yes
email nobody@nowhere.com

glm small_glm1
gmatrix testglm.G
kernel eigen1.ref 2000
noisemodel noiseparams.ref
meannorm no
driftcorrect yes
highs 1
lows 4
orderg 40
dirname testglm
scan small1.nii.gz
scan small2.nii.gz
end
EOF


cat > test.gds<< EOF
gsession testglm.G

scan small1.nii.gz
scan small2.nii.gz
TR 2000
sampling 100
length 40

newcov intercept
 type K
end

newcov scan-effect
 type I
end

newcov spike
 cov-name i_spikes
 absolute 2-5
 type I
end

newcov spike
 cov-name ni_spikes
 absolute 12-14
 option convolve eigen1.ref 2000 " [c]"
end

mean-center-all

EOF

cat > eigen1.ref<< EOF
;VB98
;REF1
      0.00000
     0.356963
     0.904650
     0.908452
     0.440000
    0.0981926
   -0.0208208
   -0.0397777
EOF

cat > noiseparams.ref<< EOF
;VB98
;REF1
2527
0.00899
0.0101
EOF

gds test.gds
mkdir -p testglm
cp ../anat.nii.gz testglm/display.nii.gz
VOXBO_CORES=all vbmakeglm test.glm
VOXBO_CORES=all vbpermgen -m testglm/testglm -d perm -t 1 -c "scanfx-1" -p "0 0 0" -n 10 -s 763943763
#vbstatmap testglm -c "foo t vec 1 0 0 0 0" -o testmap.nii.gz
#vbcmp testmap.nii.gz ref_t.nii.gz
