
# Makefile for gds

-include ../make_vars.txt
include ../make_stuff.txt

VBLIBS=$(VBLIBS2)

# place to search for includes and targets
ONEOVERFDIR=../utils

# object files (including moc files) for the executables
GDS_OBJECTS = gds_main.o gds.o

# miscellaneous flags and such
LIBS = $(LDFLAGS) -Wl,--no-as-needed $(LIBDIRS) $(DLLIB) -lm -lvbprefs -lvbglm -lvbio -lvbutil -lz -lpng $(GSL_LIBS) -lboost_system

ALLBINS=gds
ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),vlsm)
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

$(VBLIBS):
	+make -C ../lib $@

gds: $(GDS_OBJECTS) $(VBLIBS)
	+make -C	../utils fitOneOverF.o	
	$(CXX) -o gds $(GDS_OBJECTS) $(LIBS)

gds.o: gds.cpp gds.h
	$(CXX) -c -o gds.o gds.cpp $(CXXFLAGS)

gds_main.o: gds_main.cpp
	$(CXX) -c -o gds_main.o gds_main.cpp $(CXXFLAGS)

