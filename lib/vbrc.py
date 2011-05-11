#!/usr/bin/python
# vbrc
# usage: python vbrc.py my.hlp.h varname file varname file ...
# if varname is preceded with a hyphen (-varname), then lines
#  with leading pound signs (#) are skipped

import sys,datetime
global outfile
global skipflag
global flist
skipflag=0
flist=[]


def printhelp():
    print
    print "VoxBo vbrc.py utility"
    print "summary: convert files to c declarations"
    print "usage:"
    print "  vbrc <outfile> <varname> <infile> [<varname> <infile> ...]"
    print "notes:"
    print "  varnames that are prefixed with - will be processed line-by-line,"
    print "  and lines beginning with # will be skipped"
    print

def createstamp ():
    return datetime.datetime.now().strftime("%d%b%Y_%H:%M")

def parsehlp (name,fname):
    myskipflag=skipflag
    if (name[0]=="-"):
        name=name[1:]
        myskipflag=1;
    linecnt=0
    if (fname=="-"):
        infile=sys.stdin
    else:
        infile=open(fname,"r")
    outfile.write("static const char %s[]= {\n  "%name)
    while infile:
        if (myskipflag):
            buf=infile.readline()
        else:
            buf=infile.read(16384)
        if len(buf) == 0:
            break
        if myskipflag and buf[0]=='#':
            continue        
        for ch in buf:
            if linecnt > 13:
                outfile.write("\n  ")
                linecnt=0
            outfile.write("0x%x," % ord(ch))
            linecnt=linecnt+1
    if linecnt > 13:
        outfile.write("\n  ")
    outfile.write("0x%x\n};\n" % 0)
    infile.close()

outfname=""
for arg in sys.argv[1:]:
    if arg=="-#":
        skipflag=1
    elif outfname == "":
        outfname=arg
    else:
        flist.append(arg)

cnt=len(flist)
if (cnt < 2 or cnt%2):
    printhelp()
    exit(1);

outfile=open(outfname,"w")
for ind in range(0,cnt,2):
    varname=flist[ind]
    ifilename=flist[ind+1]
    print "vbrc.py: processing",varname,"from",ifilename
    parsehlp(varname,ifilename)

outfile.close()
sys.exit(0)
