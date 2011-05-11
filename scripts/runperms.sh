#!/bin/bash

# This bash script is meant to demonstrate how we can run a simple
# permutation test.  You need to edit the following variables and then
# edit the vbvolregress line below.

DEST=perm_dir    # destination for permutations
COUNT=10         # number of permutations


for (( iter=1 ; iter<=$COUNT ; iter++ )) ; do
  echo vbregress -iv -iv -iv -dv -c \"foo t vec 1 0 0\" -o perm_dir/cube_$iter.nii.gz
done
