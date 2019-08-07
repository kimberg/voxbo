
# Makefile for vbvlsm(?)

-include ../make_vars.txt
include ../make_stuff.txt

CXXFLAGS += -DQT3_SUPPORT $(QTINCDIRS)

PERMUTATION_OBJECTS=../stand_alone/perm.o ../stand_alone/utils.o ../stand_alone/time_series_avg.o ../stand_alone/koshutil.o
OBJECTS=vbvlsm.o rsrc.o $(PERMUTATION_OBJECTS)
LIBS = $(LDFLAGS) $(LIBDIRS) $(QTLIBDIRS) $(QTLIBS) -Xlinker -lvbglm -lvbprefs -lvbio -lvbutil -lz $(DLLIB) -lgsl -lgslcblas -lboost_system

ALLBINS=vbvlsm
ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else
	BINS=
endif

ifdef VB_NOX
  BINS=
endif

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

# executables

test: test.o
	$(CXX) -o test test.o $(LIBS)

vbvlsm: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o vbvlsm $(OBJECTS) $(LIBS)

# the objects

rsrc.o : rsrc.qrc
	$(RCC) rsrc.qrc -o rsrc.cpp
	$(CXX) -c rsrc.cpp $(CXXFLAGS)

vbvlsm.o: vbvlsm.cpp
	$(MOC) vbvlsm.cpp -o vbvlsm.moc.h
	$(CXX) -c vbvlsm.cpp $(CXXFLAGS)

# custom qt widgets

vbqt_canvas.o: vbqt_canvas.moc.h vbqt_canvas.cpp
	$(CXX) -c vbqt_canvas.cpp $(CXXFLAGS)

# mocs

vbqt_canvas.moc.h: vbqt_canvas.h
	$(MOC) vbqt_canvas.h -o vbqt_canvas.moc.h
