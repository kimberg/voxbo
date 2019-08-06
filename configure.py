#!/usr/bin/python
from optparse import OptionParser
import sys,os,glob

# possible targets include: all, vlsm, spm

def printhelp():
    print "  "
    print "VoxBo configuration script (configure.py)"
    print "summary: script for running simple VLSM analyses"
    print "usage:"
    print "  configure [<flags>] [<package>]"
    print "arguments include any combination of the following:"
    print "  -a              find target directory automatically"
    print "  -s              build shared libraries (default: static)"
    print "  --nox           skip X-based stuff"
    print "  vlsm            build just the VLSM-relevant binaries"
    print "  spm             build just the SPM-relevant binaries"
    print "  nd              build the neurodebian package"
    print "  --prefix=PREFIX use PREFIX as the installation prefix"
    print "  --bindir=BINDIR use BINDIR as the binary installation location"
    print "  --libdir=LIBDIR use LIBDIR as the library installation location"
    print "notes:"
    print "  This script was not created with autoconf!"
    print "  "

parser=OptionParser() #usage="usage: %prog options...")
parser.add_option("--prefix",dest="PREFIX",default="",help="installation prefix")
parser.add_option("--bindir",dest="BINDIR",default="",help="binary installation path")
parser.add_option("--libdir",dest="LIBDIR",default="",help="library installation path")
parser.add_option("--nox",dest="nox",action="store_true",default=False,help="skip x stuff")
parser.add_option("-a","--autodir",dest='autodir',action="store_true",default=False,help="find install dir automatically")
parser.add_option("-s","--shared",dest='shared',action="store_true",default=False,help="build shared libs [default: static]")
parser.add_option("-p","--package",dest="package",default="all",help="which package to build")
parser.remove_option("-h")
parser.add_option("-h","--help",dest="helpflag",action="store_true",default=False,help="help")
(opts,args)=parser.parse_args()

if opts.helpflag:
    printhelp()
    sys.exit(0)

for arg in args:
    opts.package=arg

optstring=""

if opts.nox:
    optstring+="VB_NOX=1\n"

if opts.package=="neurodebian" or opts.package=="nd":
    print "Configured NeuroDebian subpackage"
#    opts.shared=True
    optstring+="VB_TARGET?=spm\n"
elif opts.package=="spm":
    print "Configured SPM subpackage"
    optstring+="VB_TARGET?=spm\n"
elif opts.package=="vlsm":
    print "Configured VLSM subpackage"
    optstring+="VB_TARGET?=vlsm\n"
else:
    print "Configured full package"

if opts.shared:
    print "Configured to build shared VoxBo libraries"
    optstring+="VB_SHARED=1\n"
else:
    print "Configured to build static VoxBo libraries"

if (len(opts.BINDIR)):
    optstring+="VB_BINDIR="+opts.BINDIR+"\n"
if (len(opts.LIBDIR)):
    optstring+="VB_LIBDIR="+opts.LIBDIR+"\n"

# autodir: if no prefix specified, look around
if len(opts.PREFIX):
    optstring+="VB_PREFIX="+opts.PREFIX+"\n"
    print "Using prefix",opts.PREFIX
elif opts.autodir:
    prefix="."
    if (len(opts.PREFIX)):
        prefix=opts.PREFIX
    else:
        a=glob.glob("/usr/local/[Vv]ox[Bb]o")
        a=a+glob.glob(os.environ['HOME']+"/[Vv]ox[Bb]o")
        a=a+glob.glob("../bin")
        for pref in a:
            if os.access(pref+"/bin",os.W_OK):
                prefix=pref
                break
    optstring+="VB_PREFIX="+prefix+"\n"
    print "Using prefix",prefix

if optstring=="" and os.access("make_vars.txt",os.F_OK):
    os.unlink("make_vars.txt")

if optstring!="":
    f=open("make_vars.txt","w")
    f.write(optstring)
    f.close()
