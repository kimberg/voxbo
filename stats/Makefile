
# Makefile for stat mapping code

-include ../make_vars.txt
include ../make_stuff.txt

VBLIBS=$(VBLIBS2)

# miscellaneous flags and such

LIBS = $(LDFLAGS) $(LIBDIRS) -lm -lvbprefs -lvbglm -lvbio -lvbutil -lz -lpng $(DLLIB) $(GSL_LIBS)

ALLBINS=vbvolregress vbmakeregress vbstatmap vbdumpstats glminfo vbperminfo
ALLBINS+=vbxts vbtmap vbmakeresid vbfdr vbpermvec vbscoregen vbmap

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),db)
    BINS=
else
	BINS=vbvolregress vbperminfo vbxts vbtmap vbfdr vbpermvec vbscoregen
endif

all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(ALLBINS) $(CLEANME)

$(VBLIBS):
	+make -C ../lib $@

%.hlp.h : %.hlp
	$(VBRC) $@ -myhelp $<

# THE EXECUTABLES

vbperminfo: vbperminfo.cpp vbperminfo.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbperminfo vbperminfo.cpp $(LIBS)

vbstatmap: vbstatmap.cpp glmutil.cpp vbstatmap.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbstatmap vbstatmap.cpp -lvbglm $(LIBS)

vbtmap: vbtmap.cpp vbtmap.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbtmap vbtmap.cpp $(LIBS)

glminfo: glminfo.cpp glminfo.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o glminfo glminfo.cpp -lvbglm $(LIBS)

vbmakeresid: vbmakeresid.cpp vbmakeresid.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbmakeresid vbmakeresid.cpp -lvbglm $(LIBS)

vbdumpstats: vbdumpstats.cpp vbdumpstats.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbdumpstats vbdumpstats.cpp -lvbglm $(LIBS)

vbmakeregress: vbmakeregress.cpp vbmakeregress.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbmakeregress vbmakeregress.cpp -lvbglm $(LIBS)

vbvolregress: vbvolregress.cpp vbvolregress.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbvolregress vbvolregress.cpp -lvbglm $(LIBS)

vbxts: vbxts.cpp vbxts.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbxts vbxts.cpp -lvbglm $(LIBS)

vbpermvec: vbpermvec.cpp vbpermvec.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbpermvec vbpermvec.cpp -lvbglm $(LIBS)

vbscoregen: vbscoregen.cpp vbscoregen.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbscoregen vbscoregen.cpp -lvbglm $(LIBS)

vbmap: vbmap.cpp $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbmap vbmap.cpp -lvbglm $(LIBS)

vbfdr: vbfdr.cpp vbfdr.hlp.h $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbfdr vbfdr.cpp -lvbglm $(LIBS)
