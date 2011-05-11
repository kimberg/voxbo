#!/usr/bin/python
from optparse import OptionParser
import sys,os,glob

def printhelp():
    print "  "
    print "VoxBo configuration script (configure.py)"


parser=OptionParser() #usage="usage: %prog options...")
parser.add_option("-s","--skel",dest="skel",default="skel",help="skeleton source")
parser.add_option("-d","--dest",dest="dest",default=".",help="skeleton source")
parser.remove_option("-h")
parser.add_option("-h","--help",dest="helpflag",action="store_true",default=False,help="help")
(opts,args)=parser.parse_args()


# script lines start with either once or all
# skel contains vars.txt
# skel contians scripts subdir, it's the only dir that gets replacement

# we parse vars.txt
# we create the needed number of copies
# in each copy, we create vars.sh (FOO=bar for all the vars)
# we search/replace everything in the script dir
# we go through scripts/run.txt and run it

def parsevars(dirname):
    f=open(dirname+"/vars.txt")
    lines=f.readlines()
    vars=[]
    for line in lines:
        toks=line.split()
        dd={'name':toks[0],'vals':toks[1:]}
        vars.append(dd)
    return vars

def makecombos(dicts):
    if len(dicts)<1: return [{}]
    varsets=[]
    dd=dicts[0]
    for value in dicts[0]['vals']:
        varsets.append(dict([(dicts[0]['name'],value)]))
    for dd in dicts[1:]:
        vnew=[]
        for ss in varsets:
            for value in dd['vals']:
                tmpset=ss.copy()
                tmpset[dd['name']]=value
                vnew.append(tmpset)
        varsets=vnew
    return varsets

def makerepdir(skel,dest,combo):
    dname="iter"
    for a,v in combo.iteritems():
        dname+="_"+v
    os.system("mkdir "+dname)
    os.system("cp -r "+skel+"/* "+dname)


def main():
    skel=opts.skel
    dest=opts.dest
    print "dirname: %s" % skel
    print "   dest: %s" % dest
    varset=parsevars(skel)
    #for vv in varset: print vv['name']," ",vv['vals']
    combos=makecombos(varset)
    #for combo in combos: print combo
    for combo in combos:
        makerepdir(skel,dest,combo)
    # make all the directories
    # create vars.sh
    # do variable replacement in scripts/*
    # parse and utilize run.txt






main()
