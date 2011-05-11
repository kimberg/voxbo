#!/bin/bash2

dir=$*

for foo in $dir ; do
 echo $foo
done

PWD=`pwd`
BPWD=`basename $PWD`

if (( $# == 1)) ; then
  ADIR=$1 ;
elif [ "$BPWD" == "Anatomy" ] ; then
  ADIR="."
elif [ -d "Anatomy" ] ; then
  ADIR="Anatomy"
else 
  echo 'argh'
  exit 255
fi


echo $ADIR
