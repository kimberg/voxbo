
# Makefile for getput i/o code
# original version by Zack Smith

-include ../make_vars.txt
include ../make_stuff.txt

all:	getdata putdata

getdata:	getdata.cpp time_series_avg.cpp  $(VBLIBS)
	${CXX} -I../stand_alone ${CXXFLAGS}  -Wall getdata.cpp time_series_avg.cpp $(LDFLAGS) -lvbio -lvbutil -lvbglm -lgsl -lgslcblas $(DLLIB) -o getdata

putdata:	putdata.cpp $(VBLIBS)
	${CXX} ${CXXFLAGS}  -Wall putdata.cpp $(LDFLAGS) -lvbio -lvbutil -lvbglm -lgsl -lgslcblas $(DLLIB) -o putdata

clean:	
	rm -f /tmp/fifo getdata putdata *~ *.o *.exe

install:
	cp -f getdata putdata $(VB_BINDIR)
