#!/bin/bash

# VoxBo install script

# some defaults
DEST=/usr/local/VoxBo
VOXBOUSER=voxbo

printhelp() {
  echo "  "
  echo "VoxBo install"
  echo "summary: minimal VoxBo install tasks"
  echo "usage:"
  echo "  makevlsm <flags>"
  echo "the following flags are honored:"
  echo "  -d <dest>   destination directory (default: /usr/local/VoxBo)"
  echo "  -u <user>   VoxBo user (default: voxbo)"
  echo "notes:"
  echo "  This script sets proper permissions for a VoxBo installation and"
  echo "  creates some minimal files if missing.  It doesn't overwrite existing"
  echo "  configuration files, so you can use it to fix an existing installation"
  echo "  or to initialize a new one."
  echo "  "
}

while getopts hd:u: opt; do
  case "$opt" in
    h) printhelp; exit;  ;;
#    b) BATCH=1 ;;
    d) DEST=$OPTARG ;;
    u) VOXBOUSER=$OPTARG ;;
    \\?) echo "argh!" ;;
  esac
done

# set temporary umask for anything we create
umask 022

# make sure major directories are in place
mkdir -p $DEST/bin $DEST/drop $DEST/elements $DEST/etc $DEST/misc $DEST/pros\
         $DEST/queue $DEST/scripts $DEST/etc/logs $DEST/etc/servers $DEST/etc/jobtypes
if (($? > 0)) ; then
  echo "VoxBo install failed creating directories"
  exit 109
fi

###################### defaults

if [[ ! -e $DEST/etc/defaults ]] ; then
echo "creating defaults"
cat > $DEST/etc/defaults <<EOF
# This is a minimal defaults file created by the install script.
EOF
fi

###################### serverlist

if [[ ! -e $DEST/etc/serverlist ]] ; then
echo "creating serverlist"
cat > $DEST/etc/serverlist <<EOF
# create a line like the following for each server
localhost localhost.localdomain
EOF
fi

###################### servers/sample

if [[ ! -e $DEST/etc/servers/sample ]] ; then
echo "creating servers/sample"
cat > $DEST/etc/servers/sample <<EOF
name sample
hostname sample.myuniversity.edu
speed 1800
# avail lines have days, hours, priority, and ncpus
avail 1-5 8-17 2 1  ;; pri 2 weekdays
avail 1-5 18-7 1 1  ;; pri 1 weeknights
avail 6-0 0-23 1 1  ;; pri 1 weekends
provides cpu
provides mailagent
EOF
fi

###################### queue/vb.num

if [[ ! -e $DEST/queue/vb.num ]] ; then
echo "creating queue/vb.num"
cat > $DEST/queue/vb.num <<EOF
1
EOF
fi

###################### etc/jobtypes/shellcommant.vjt

if [[ ! -e $DEST/etc/jobtypes/shellcommand.vjt ]] ; then
echo "creating etc/jobtypes/shellcommand.vjt"
cat > $DEST/etc/jobtypes/shellcommand.vjt <<EOF
shortname shellcommand
description execute any command line
invocation commandline

argument
 name=command
 argdesc=the entire line to be run from the command line
end

command /bin/sh -c "$(command)"
requires cpu
EOF
fi

# permissions

echo "setting permissions and ownership"
chown -R $VOXBOUSER $DEST
chmod -R a+rX $DEST
chmod a+rwx $DEST/drop

echo "done!"

exit
