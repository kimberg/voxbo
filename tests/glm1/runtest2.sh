#!/bin/sh

starttime=`date +%s`

# create data

vbsim -d 20 20 20 200 -n 10 5 10 -o big1.nii.gz -s 8972348
vbsim -d 20 20 20 200 -n 10 5 10 -o big2.nii.gz -s 6547834
vbsim -d 20 20 20 200 -n 10 5 10 -o big3.nii.gz -s 1378238

cat > test2.glm<< EOF
pieces 4
audit yes
email nobody@nowhere.com

glm testglm2
gmatrix testglm2.G
#kernel
#noisemodel
meannorm no
driftcorrect yes
highs 1
lows 4
orderg 600
dirname testglm2
scan big1.nii.gz
scan big2.nii.gz
scan big3.nii.gz
end
EOF


cat > test2.gds<< EOF
gsession testglm2.G

scan big1.nii.gz
scan big2.nii.gz
scan big3.nii.gz
TR 2000
sampling 100
length 600

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

gds test2.gds
mkdir -p testglm2
vbim -newvol 20 20 20 1 short -add 1 -write testglm2/display.nii.gz

VOXBO_CORES=4 vbmakeglm test2.glm
VOXBO_CORES=4 vbpermgen -m testglm2/testglm2 -d perm -t 1 -c "scanfx-1" -p "0 0 0" -n 2
