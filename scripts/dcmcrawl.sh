#!/bin/bash
# crawl directories, using dcmsplit to anonymize data

printhelp() {
  echo "  "
  echo "VoxBo dcmcrawl"
  echo "summary: crawl directories, running dcmsplit on all the files"
  echo "usage:"
  echo "  dcmcrawl <workingdir> <target>"
  echo "the following flags are honored:"
  echo "notes:"
  echo "  "
}

if [[ $# -lt 2 ]] ; then
  printhelp;
  exit;
fi

STAMPDIR=$1
FILESYSTEM=$2
FNAME=`echo ${FILESYSTEM} | tr "/" "_"`
DATE=`date +%Y_%m_%d`
LOGFILE=${STAMPDIR}/${FNAME}_${DATE}

if [[ -e ${STAMPDIR}/${FNAME}_crawl1.dat ]] ; then
  T1=${STAMPDIR}/${FNAME}_crawl1.dat
  T2=${STAMPDIR}/${FNAME}_crawl2.dat
  PAT="-cnewer ${T1}"
elif [[ -e ${STAMPDIR}/${FNAME}_crawl2.dat ]] ; then
  T2=${STAMPDIR}/${FNAME}_crawl1.dat
  T1=${STAMPDIR}/${FNAME}_crawl2.dat
  PAT="-cnewer ${T1}"
else
  T2=${STAMPDIR}/${FNAME}_crawl1.dat
  PAT="-readable"
#  find /${FILESYSTEM} -name \* -print | xargs -r file
fi

touch ${T2}
find /${FILESYSTEM} ${PAT} -type f -print | xargs -r dcmsplit -m > ${LOGFILE}

# log the output of dcmsplit

if [[ -n ${T1} ]] ;then
  rm ${T1}
fi
