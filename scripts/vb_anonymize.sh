


# maybe use susan???

# register directly to ch2 using ANTS
ANTS 3 -m PR["ch2.nii.gz","mprage.nii.gz",1,3] -i 2x1x1 -o STRIP -t SyN[0.5] -r Gauss[3,0]

# warp to ch2 just to see if it worked
WarpImageMultiTransform 3 mprage.nii.gz wmprage.nii.gz -R ch2.nii.gz STRIPWarp STRIPAffine.txt
#WarpImageMultiTransform 3 ch2.nii.gz wch2.nii.gz -R mprage.nii.gz STRIPWarp STRIPAffine.txt

# ch2mask created by using spm8segment on ch2 (!), then using vbim thusly:
# vbim c[123]* -convert float -sum -quantize 1 -info -smoothvox 3 3 3 -info\
#      -thresh 0.2 -quantize 1 -convert byte -write sch2mask.nii

# map ch2's gray/white/csf map back onto our image
WarpImageMultiTransform 3 sch2mask.nii smask.nii.gz -R mprage.nii.gz -i STRIPAffine.txt STRIPInverseWarp

# blur and quantize again, and mask original image using vbim
vbim smask.nii.gz -thresh 0.2 -quantize 1 -smoothvox 3 3 3 -thresh 0.2 -quantize 1 -convert byte -write smask2.nii.gz

# inspect!

# apply smask2 to mprage
vbim mprage.nii.gz -mask smask2.nii.gz -write mmmprage.nii.gz


# inspect again!

