
# Makefile for 1/f fitting, vbcfx, gcheck and sortmvpm

-include ../make_vars.txt
include ../make_stuff.txt

# Some variables
MYLIBVOXBO=../lib

LIBS = $(LDFLAGS) $(LIBDIRS) -lvbglm -lvbprefs -lvbio -lvbutil -lz -lpng $(DLLIB) $(GSL_LIBS)

ALLBINS=vbfit vbcfx txt2num gcheck sortmvpm
ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=vbfit vbcfx gcheck
else
	BINS=
endif

# standard targets

all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(ALLBINS)

# executables and such

txt2num:
	ln -sf ./vbcfx txt2num

vbfit: fitOneOverF.o vbfit.o
	$(CXX) $(CXXFLAGS) -o vbfit fitOneOverF.o vbfit.o $(LIBS)

vbcfx: vbcfx.o
	$(CXX) $(CXXFLAGS) -o vbcfx vbcfx.o $(LIBS)	

gcheck: gcheck.o
	$(CXX) $(CXXFLAGS) -o gcheck gcheck.o $(LIBS)	

sortmvpm: sortmvpm.o
	$(CXX) $(CXXFLAGS) -o sortmvpm sortmvpm.o $(LIBS)

vbfit.o: vbfit.cpp fitOneOverF.h
	$(CXX) $(CXXFLAGS) -c vbfit.cpp -o vbfit.o

fitOneOverF.o: fitOneOverF.cpp fitOneOverF.h
	$(CXX) $(CXXFLAGS) -c fitOneOverF.cpp -o fitOneOverF.o

vbcfx.o: vbcfx.cpp
	$(CXX) $(CXXFLAGS) -c vbcfx.cpp -o vbcfx.o

gcheck.o: gcheck.cpp
	$(CXX) $(CXXFLAGS) -c gcheck.cpp -o gcheck.o

sortmvpm.o: sortmvpm.cpp
	$(CXX) $(CXXFLAGS) -c sortmvpm.cpp -o sortmvpm.o
