#!/usr/bin/python
# fdrscript.sh

# The FDR-controlling threshold (see Genovese et al., 2002)
# corresponds to the largest p value that is less than i/V * q, where
# q is the desired false discovery rate and i is the index of the p
# value in the sorted list of all p values from 1 to V.

# This script takes a vector of (we hope) p values, sanity checks it,
# sorts it, and then graphs it against the FDR line using matplotlib.

from pylab import *

y=[]
f=open('pfile.ref','r')
unconv=0
tmp=0.0
for line in f:
    try:
        tmp=float(line)
    except ValueError:
        print '[W] plotfdr: unconverted line: '+line.strip()
        unconv=unconv+1
        if unconv > 8:
            print '[E] plotfdr: too many non-numeric values, exiting'
            exit(101)
        continue
    if (tmp == nan or tmp==inf or tmp==-inf):
        print '[E] plotfdr: encountered non-finite value, exiting'
        exit(101)
    if tmp>1.0 or tmp<0.0:
        print '[E] plotfdr: found invalid p value '+line.strip()+', exiting'
        exit(100)
    y.append(float(line))

f.close()

print 'read',len(y),'values'

y.sort()

x=arange(1,len(y)+1,1)
plot(x,y,linewidth=1.0)

y=0.01*x/len(x)
plot(x,y,linewidth=0.5)

y=0.05*x/len(x)
plot(x,y,linewidth=0.5)

xlabel('ordinal position of p value')
ylabel('actual p value')
title('haha')
grid(True)
show()
