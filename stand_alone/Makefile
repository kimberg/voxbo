
# Makefile for stand-alone stuff

-include ../make_vars.txt
include ../make_stuff.txt

VBLIBS=$(VBLIBS2)

CALCGSPS_OBJECTS=calc_gs_ps.o timeseries.o utils.o koshutil.o
SLICE_ACQ_OBJECTS=utils.o koshutil.o
TESPLIT_OBJECTS=utils.o koshutil.o
TESJOIN_OBJECTS=$(TESPLIT_OBJECTS)
REGRESSION_OBJECTS=regression.o utils.o koshutil.o
PERMUTATION_OBJECTS=perm.o utils.o time_series_avg.o koshutil.o

LIBS=$(LDFLAGS) $(LIBDIRS) -lvbglm -lvbprefs -lvbio -lvbutil -lz -lpng $(GSL_LIBS) $(DLLIB)

ALLBINS= calcgs calcps sliceacq tesplit tesjoin makematkg makematk\
		comptraces permstep vbregress vbpermmat
OUT=tesregstart tesregfinish tesregstep assembleps vbvolregress

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=$(ALLBINS)
else
	BINS=vbpermmat
endif

# the main targets

all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(ALLBINS)

$(VBLIBS):
	+make -C ../lib $@

# THE SHARED OBJECT
X_LD_FLAGS= -shared --warn-once  -Xlinker -noinhibit-exec
LIB_VOXBO=../lib
ifndef $(IDL_DIR)
  IDL_DIR=/usr/local/rsi/idl/
endif
IDL_VERSION_MAJOR=$(shell grep IDL_VERSION_MAJOR $(IDL_DIR)/external/export.h | cut -d" " -f3)
IDL_VERSION_MINOR=$(shell grep IDL_VERSION_MINOR $(IDL_DIR)/external/export.h | cut -d" " -f3)
INCLUDES= -I./ -I ../lib -I $(IDL_DIR)/external
INCLUDE_FILES= $(LIB_VOXBO)/genericexcep.h $(LIB_VOXBO)/stringtokenizer.h $(LIB_VOXBO)/vbutil.h vb_common_incs.h time_series_avg.h

$(LIB_VOXBO)/vbutil.o: $(LIB_VOXBO)/vbutil.cpp $(LIB_VOXBO)/vbutil.h
	$(CXX) $(CXXFLAGS) -o $(@) $(LIB_VOXBO)/$(@F:.o=.cpp) $(INCLUDES)
$(LIB_VOXBO)/stringtokenizer.o: $(LIB_VOXBO)/stringtokenizer.cpp $(LIB_VOXBO)/stringtokenizer.h $(LIB_VOXBO)/genericexcep.h
	$(CXX) $(CXXFLAGS) -o $(@) $(LIB_VOXBO)/$(@F:.o=.cpp) $(INCLUDES)
$(LIB_VOXBO)/genericexcep.o: $(LIB_VOXBO)/genericexcep.cpp $(LIB_VOXBO)/genericexcep.h
	$(CXX) $(CXXFLAGS) -o $(@) $(LIB_VOXBO)/$(@F:.o=.cpp) $(INCLUDES)

# THE EXECUTABLES

calcgs: $(CALCGSPS_OBJECTS) calcgs.cpp
	$(CXX) -o $(@) $(@).cpp $(CALCGSPS_OBJECTS) $(CXXFLAGS) $(LIBS)

calcps: $(CALCGSPS_OBJECTS) calcps.cpp
	$(CXX) -o $(@) $(@).cpp $(CALCGSPS_OBJECTS) $(CXXFLAGS) $(LIBS)

sliceacq: $(SLICE_ACQ_OBJECTS) sliceacq.cpp
	$(CXX) -o $(@) $(@).cpp $(SLICE_ACQ_OBJECTS) $(CXXFLAGS) $(LIBS)

tesplit: $(TESPLIT_OBJECTS) tesplit.cpp
	$(CXX) -o $(@) $(@).cpp $(TESPLIT_OBJECTS) $(CXXFLAGS) $(LIBS)

tesjoin: $(TESJOIN_OBJECTS) tesjoin.cpp
	$(CXX) -o $(@) $(@).cpp $(TESJOIN_OBJECTS) $(CXXFLAGS) $(LIBS)

tesregstep: $(REGRESSION_OBJECTS) tesregstep.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

tesregstart: $(REGRESSION_OBJECTS) tesregstart.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

tesregfinish: $(REGRESSION_OBJECTS) tesregfinish.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

makematkg: $(REGRESSION_OBJECTS) makematkg.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

makematk: $(REGRESSION_OBJECTS) makematk.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

assembleps: $(REGRESSION_OBJECTS) assembleps.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

comptraces: $(REGRESSION_OBJECTS) comptraces.cpp
	$(CXX) -o $(@) $(@).cpp $(REGRESSION_OBJECTS) $(CXXFLAGS) $(LIBS)

permstep: $(PERMUTATION_OBJECTS) permstep.cpp
	$(CXX) -o $(@) $(@).cpp $(PERMUTATION_OBJECTS) $(CXXFLAGS) $(LIBS)

vbpermmat: $(PERMUTATION_OBJECTS) vbpermmat.cpp
	$(CXX) -o $(@) $(@).cpp $(PERMUTATION_OBJECTS) $(CXXFLAGS) $(LIBS)

vbregress: vbregress.o $(VBLIBS)
	$(CXX) -o vbregress vbregress.o $(CXXFLAGS) $(LIBS)

# THE TOPLEVEL OBJECTS

koshutil.o: koshutil.cpp koshutil.h
	$(CXX) -c  koshutil.cpp $(CXXFLAGS)

utils.o: utils.cpp utils.h
	$(CXX) -c  utils.cpp $(CXXFLAGS)

calc_gs_ps.o: calc_gs_ps.cpp calc_gs_ps.h $(VBLIBS)
	$(CXX) -c  calc_gs_ps.cpp $(CXXFLAGS)

timeseries.o: timeseries.cpp timeseries.h
	$(CXX) -c timeseries.cpp $(CXXFLAGS)

regression.o: regression.cpp regression.h
	$(CXX) -c regression.cpp $(CXXFLAGS)

vbregress.o: vbregress.cpp
	$(CXX) -c vbregress.cpp $(CXXFLAGS)

time_series_avg.o: time_series_avg.cpp time_series_avg.h
	$(CXX) -c time_series_avg.cpp $(CXXFLAGS)

perm.o: perm.cpp perm.h
	$(CXX) -c perm.cpp $(CXXFLAGS)
