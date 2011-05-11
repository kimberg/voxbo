
# Makefile for crunch

-include ../make_vars.txt
include ../make_stuff.txt

# object files for the executables

TESTOBJECTS = test.o
VBMM2OBJECTS = vbmm2.o
VOXSURFOBJECTS = voxsurf.o
MGLMOBJECTS = vbmakeglm.o
REALIGNOBJECTS = realign.o realignutils.o utils.o smoothutils.o matrixutils.o\
	crunchcube.o
SMOOTHOBJECTS = utils.o smoothutils.o matrixutils.o crunchcube.o
NORMOBJECTS = norm.o realignutils.o utils.o smoothutils.o matrixutils.o\
	norm_utils.o norm_main.o norm_brainwarp.o crunchcube.o
PERFMASKOBJECTS = perfmask.o realignutils.o utils.o smoothutils.o matrixutils.o\
	norm_utils.o norm_main.o norm_brainwarp.o crunchcube.o

# miscellaneous flags and such

INCDIRS += -I/usr/local/include/octave
LIBDIRS += -L/usr/local/lib/octave

LIBS =$(LDFLAGS) $(LIBDIRS) -lm -lvbglm -lvbprefs -lvbio -lvbutil -lz -lpng $(DLLIB) $(GSL_LIBS)
OCTLIBS = $(LIBS) -loctave -lcruft $(FORTLIB)

ALLBINS=vbmm2 vbmakeglm vbmakefilter #realign norm

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=$(ALLBINS)
else
	BINS=
endif

# the main targets

all: $(BINS)

install:
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(ALLBINS)

# rule to build the libvoxbo libraries

$(VBLIBS):
	+make -C ../lib $@

# THE EXECUTABLES

realign: $(REALIGNOBJECTS) $(VBLIBS)
	$(CXX) -o realign $(REALIGNOBJECTS) $(OCTLIBS)

smooth: $(SMOOTHOBJECTS) $(VBLIBS)
	$(CXX) -o smooth $(SMOOTHOBJECTS) $(LIBS)

test: $(TESTOBJECTS) $(VBLIBS)
	$(CXX) -o vbtest $(TESTOBJECTS) $(LIBS)

norm: $(NORMOBJECTS) $(VBLIBS)
	$(CXX) -o norm $(NORMOBJECTS) $(OCTLIBS)

perfmask: $(PERFMASKOBJECTS) $(VBLIBS)
	$(CXX) -o perfmask $(PERFMASKOBJECTS) $(LIBS)

vbmakeglm: $(MGLMOBJECTS) $(VBLIBS)
	$(CXX) -o vbmakeglm $(MGLMOBJECTS) -lvbprefs $(LIBS)

vbmakefilter: vbmakefilter.o $(VBLIBS)
	$(CXX) -o vbmakefilter vbmakefilter.o -lvbprefs $(LIBS)

vbmm2: $(VBMM2OBJECTS) $(VBLIBS)
	$(CXX) -o vbmm2 $(VBMM2OBJECTS) $(LIBS) $(GSL_LIBS)

# THE TOPLEVEL OBJECTS

vbmm2.o: vbmm2.cpp $(VBLIBS)
	$(CXX) -c -o vbmm2.o vbmm2.cpp $(CXXFLAGS)

vbmakeglm.o: vbmakeglm.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o vbmakeglm.o vbmakeglm.cpp $(CXXFLAGS)

vbmakefilter.o: vbmakefilter.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o vbmakefilter.o vbmakefilter.cpp $(CXXFLAGS)

realign.o: realign.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o realign.o realign.cpp $(CXXFLAGS)

smooth.o: smooth.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o smooth.o smooth.cpp $(CXXFLAGS)

voxsurf.o: voxsurf.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o voxsurf.o voxsurf.cpp $(CXXFLAGS) $(VSFLAGS)

test.o: test.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o test.o test.cpp $(CXXFLAGS)

norm.o: norm.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o norm.o norm.cpp $(CXXFLAGS)

perfmask.o: perfmask.cpp vbcrunch.h $(VBLIBS)
	$(CXX) -c -o perfmask.o perfmask.cpp $(CXXFLAGS)

# THE UTIL OBJECTS

realignutils.o: realignutils.cpp vbcrunch.h
	$(CXX) -c -o realignutils.o realignutils.cpp $(CXXFLAGS)

smoothutils.o: smoothutils.cpp vbcrunch.h
	$(CXX) -c -o smoothutils.o smoothutils.cpp $(CXXFLAGS)

utils.o: utils.cpp vbcrunch.h
	$(CXX) -c -o utils.o utils.cpp $(CXXFLAGS)

matrixutils.o: matrixutils.cpp vbcrunch.h
	$(CXX) -c -o matrixutils.o matrixutils.cpp $(CXXFLAGS)

crunchcube.o: crunchcube.cpp vbcrunch.h
	$(CXX) -c -o crunchcube.o crunchcube.cpp $(CXXFLAGS)

norm_main.o: norm_main.cpp vbcrunch.h
	$(CXX) -c -o norm_main.o norm_main.cpp $(CXXFLAGS)

norm_utils.o: norm_utils.cpp vbcrunch.h
	$(CXX) -c -o norm_utils.o norm_utils.cpp $(CXXFLAGS)

norm_brainwarp.o: norm_brainwarp.cpp vbcrunch.h
	$(CXX) -c -o norm_brainwarp.o norm_brainwarp.cpp $(CXXFLAGS)

