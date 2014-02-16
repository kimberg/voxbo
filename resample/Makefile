
# Makefile for resample

-include ../make_vars.txt
include ../make_stuff.txt

# object files for the executables

RESAMPLEOBJECTS = resample.o
VBTRANSFORMOBJECTS = vbtransform.o

# miscellaneous flags and such

LIBS = $(LDFLAGS) -Wl,--no-as-needed $(LIBDIRS) -lm -lvbprefs -lvbio -lvbutil -lz -lpng $(DLLIB) $(GSL_LIBS) -lboost_system

# resample is in all packages
BINS=resample
OUT=vbtransform

# the main targets

all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(BINS)

$(VBLIBS):
	+make -C ../lib $@

# THE EXECUTABLES

resample: $(RESAMPLEOBJECTS) $(VBLIBS)
	$(CXX) -o resample $(RESAMPLEOBJECTS) $(LIBS)

vbtransform: $(VBTRANSFORMOBJECTS) $(VBLIBS)
	$(CXX) -o vbtransform $(VBTRANSFORMOBJECTS) $(LIBS)

# THE OBJECTS

resample.o: resample.cpp $(VBLIBS)
	$(CXX) $(CXXFLAGS) -c -o resample.o resample.cpp

vbtransform.o: vbtransform.cpp $(VBLIBS)
	$(CXX) $(CXXFLAGS) -c -o vbtransform.o vbtransform.cpp
