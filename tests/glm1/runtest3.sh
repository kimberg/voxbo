#!/bin/sh

starttime=`date +%s`

# create data


vbsim -d 20 20 20 1000 -n 10 5 10 -o big1.nii.gz -s 463487334

cat > test3.glm<< EOF
pieces 4
audit yes
email nobody@nowhere.com

glm testglm3
gmatrix testglm3.G
#kernel
#noisemodel
meannorm no
driftcorrect yes
highs 1
lows 4
orderg 1000
dirname testglm3
scan big1.nii.gz
end
EOF


cat > test3.gds<< EOF
gsession testglm3.G

scan big1.nii.gz
TR 2000
sampling 100
length 1000

newcov intercept
 type K
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

gds test3.gds
mkdir -p testglm3
vbim -newvol 20 20 20 1 short -add 1 -write testglm3/display.nii.gz

VOXBO_CORES=4 vbmakeglm test3.glm
VOXBO_CORES=4 vbpermgen -m testglm3/testglm3 -d perm -t 1 -c "i_spikes" -p "0 0 0" -n 2
