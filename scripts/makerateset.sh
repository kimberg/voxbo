#!/bin/bash

LEFTFILE=ch2.nii.gz
RIGHTFILE=anat.nii.gz

printhelp() {
  echo "makerateset.sh template patient masks..."
}

if [[ $# -lt 3 ]] ; then
  printhelp;
  exit;
fi

for file in ${@} ; do
  echo image ${file%%.*} $file
done

echo

echo left ${1%%.*}
echo right ${2%%.*}

echo

for file in ${@:3} ; do
  echo mask ${file%%.*}
done

echo
echo randomize
