
# Makefile for munging functions

-include ../make_vars.txt
include ../make_stuff.txt

VBLIBS=$(VBLIBS2)

# miscellaneous flags and such

LIBS =$(LDFLAGS) -Wl,--no-as-needed $(LIBDIRS) $(LIBPATHS) -lvbglm -lvbprefs -lvbio -lvbutil -lz -lpng $(DLLIB) $(GSL_LIBS) -lboost_system

CONVERTERS=tes2cub vb2cub vb2img vb2imgs vb2tes vb2vmp vbconv
MUNGERS=vbmunge vbcmp vbshift vbsmooth vbmaskmunge vecsplit vbinterpolate vbthresh
DESIGNERS=vbprep vbbatch
STATTERS=sumrfx calcperf
MISC=cub2pngs setorigin fillmask vbhdr vbsim vbregion vborient
MISC+=vbi vbmerge4d vbvec2hdr vbse vbmaskinfo
MISC+=vboverlap vbimagemunge vbim vbfilter vbmaskcompare vbtool
XXOUT=vbmakerfx extractmask

# the main targets

ALLBINS=$(CONVERTERS) $(MUNGERS) $(DESIGNERS) $(STATTERS) $(MISC)

ifeq ($(VB_TARGET),spm)
	BINS=$(CONVERTERS) $(MUNGERS) $(MISC)
else ifeq ($(VB_TARGET),vlsm)
	BINS=vbbatch
else ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else
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

# rule to build the libvoxbo libraries

$(VBLIBS):
	+make -C ../lib $@

%.hlp.h : %.hlp
	$(VBRC) $@ -myhelp $<

vbtool.hlp.h : vbtool.hlp contrast.hlp priority.hlp
	$(VBRC) $@ -myhelp vbtool.hlp -contrasthelp contrast.hlp -priorityhelp priority.hlp

vbprep.hlp.h : vbpsample.hlp
	$(VBRC) $@ vbpsample vbpsample.hlp -myhelp vbprep.hlp

vbim.hlp.h : vbim.hlp vbim_examples.hlp
	$(VBRC) $@ -myhelp vbim.hlp -myhelp_examples vbim_examples.hlp

# THE EXECUTABLES

vbmunge: vbmunge.cpp vbmunge.hlp.h $(VBLIBS)
	$(CXX) -o vbmunge vbmunge.cpp $(LIBS) $(CXXFLAGS)

vbthresh: vbthresh.cpp vbthresh.hlp.h $(VBLIBS)
	$(CXX) -o vbthresh vbthresh.cpp $(LIBS) $(CXXFLAGS)

fillmask: fillmask.cpp fillmask.hlp.h $(VBLIBS)
	$(CXX) -o fillmask fillmask.cpp $(LIBS) $(CXXFLAGS)

vbi: vbi.cpp vbi.hlp.h $(VBLIBS)
	$(CXX) -o vbi vbi.cpp $(LIBS) $(CXXFLAGS)

vbconv: vbconv.cpp vbconv.hlp.h $(VBLIBS)
	$(CXX) -o vbconv vbconv.cpp $(LIBS) $(CXXFLAGS)

tes2cub: tes2cub.cpp tes2cub.hlp.h $(VBLIBS)
	$(CXX) -o tes2cub tes2cub.cpp $(LIBS) $(CXXFLAGS)

vb2cub: vb2cub.cpp vb2cub.hlp.h $(VBLIBS)
	$(CXX) -o vb2cub vb2cub.cpp $(LIBS) $(CXXFLAGS)

vb2img: vb2img.cpp vb2img.hlp.h $(VBLIBS)
	$(CXX) -o vb2img vb2img.cpp $(LIBS) $(CXXFLAGS)

vb2imgs: vb2imgs.cpp vb2imgs.hlp.h $(VBLIBS)
	$(CXX) -o vb2imgs vb2imgs.cpp $(LIBS) $(CXXFLAGS)

vb2tes: vb2tes.cpp vb2tes.hlp.h $(VBLIBS)
	$(CXX) -o vb2tes vb2tes.cpp $(LIBS) $(CXXFLAGS)

vbmerge4d: vbmerge4d.cpp vbmerge4d.hlp.h $(VBLIBS)
	$(CXX) -o vbmerge4d vbmerge4d.cpp $(LIBS) $(CXXFLAGS)

vb2vmp: vb2vmp.cpp vb2vmp.hlp.h $(VBLIBS)
	$(CXX) -o vb2vmp vb2vmp.cpp $(LIBS) $(CXXFLAGS)

vbcmp: vbcmp.cpp vbcmp.hlp.h $(VBLIBS)
	$(CXX) -o vbcmp vbcmp.cpp $(LIBS) $(CXXFLAGS)

cub2pngs: cub2pngs.cpp cub2pngs.hlp.h $(VBLIBS)
	$(CXX) -o cub2pngs cub2pngs.cpp $(LIBS) -lpng -lz $(CXXFLAGS)

setorigin: setorigin.cpp setorigin.hlp.h $(VBLIBS)
	$(CXX) -o setorigin setorigin.cpp $(LIBS) $(CXXFLAGS)

extractmask: extractmask.cpp extractmask.hlp.h $(VBLIBS)
	$(CXX) -o extractmask extractmask.cpp $(LIBS) $(CXXFLAGS)

vbinterpolate: vbinterpolate.cpp vbinterpolate.hlp.h $(VBLIBS)
	$(CXX) -o vbinterpolate vbinterpolate.cpp $(LIBS) $(CXXFLAGS)

sumrfx: sumrfx.cpp sumrfx.hlp.h $(VBLIBS)
	$(CXX) -o sumrfx sumrfx.cpp $(LIBS) $(CXXFLAGS)

calcperf: calcperf.cpp calcperf.hlp.h $(VBLIBS)
	$(CXX) -o calcperf calcperf.cpp $(LIBS) $(CXXFLAGS)

vbmakerfx: vbmakerfx.cpp vbmakerfx.hlp.h $(VBLIBS)
	$(CXX) -o vbmakerfx vbmakerfx.cpp $(LIBS) $(CXXFLAGS)

vbbatch: vbbatch.cpp vbbatch.hlp.h $(VBLIBS)
	$(CXX) -o vbbatch vbbatch.cpp $(LIBS) $(CXXFLAGS)

vbtool: vbtool.cpp vbtool.hlp.h $(VBLIBS)
	$(CXX) -o vbtool vbtool.cpp $(LIBS) $(CXXFLAGS)

vbhdr: vbhdr.cpp vbhdr.hlp.h $(VBLIBS)
	$(CXX) -o vbhdr vbhdr.cpp $(LIBS) $(CXXFLAGS)

vbperminfo: vbperminfo.cpp vbperminfo.hlp.h $(VBLIBS)
	$(CXX) -o vbperminfo vbperminfo.cpp $(LIBS) $(CXXFLAGS)

vbregion: vbregion.cpp vbregion.hlp.h $(VBLIBS)
	$(CXX) -o vbregion vbregion.cpp $(LIBS) $(CXXFLAGS)

vbshift: vbshift.cpp vbshift.hlp.h $(VBLIBS)
	$(CXX) -o vbshift vbshift.cpp $(LIBS) $(CXXFLAGS)

vbsmooth: vbsmooth.cpp vbsmooth.hlp.h $(VBLIBS)
	$(CXX) -o vbsmooth vbsmooth.cpp $(LIBS) $(CXXFLAGS)

vbsim: vbsim.cpp vbsim.hlp.h $(VBLIBS)
	$(CXX) -o vbsim vbsim.cpp $(LIBS) $(CXXFLAGS)

vborient: vborient.cpp vborient.hlp.h $(VBLIBS)
	$(CXX) -o vborient vborient.cpp $(LIBS) $(CXXFLAGS)

vbmaskmunge: vbmaskmunge.cpp vbmaskmunge.hlp.h $(VBLIBS)
	$(CXX) -o vbmaskmunge vbmaskmunge.cpp $(LIBS) $(CXXFLAGS)

vbimagemunge: vbimagemunge.cpp vbimagemunge.hlp.h $(VBLIBS)
	$(CXX) -o vbimagemunge vbimagemunge.cpp $(LIBS) $(CXXFLAGS)

vbim: vbim.cpp vbim.hlp.h $(VBLIBS)
	$(CXX) -o vbim vbim.cpp $(LIBS) $(CXXFLAGS)

vbfilter: vbfilter.cpp vbfilter.hlp.h $(VBLIBS)
	$(CXX) -o vbfilter vbfilter.cpp $(LIBS) $(CXXFLAGS)

vbmaskcompare: vbmaskcompare.cpp vbmaskcompare.hlp.h $(VBLIBS)
	$(CXX) -o vbmaskcompare vbmaskcompare.cpp $(LIBS) $(CXXFLAGS)

vbmaskinfo: vbmaskinfo.cpp vbmaskinfo.hlp.h $(VBLIBS)
	$(CXX) -o vbmaskinfo vbmaskinfo.cpp $(LIBS) $(CXXFLAGS)

vboverlap: vboverlap.cpp vboverlap.hlp.h $(VBLIBS)
	$(CXX) -o vboverlap vboverlap.cpp $(LIBS) $(CXXFLAGS)

vbprep: vbprep.cpp vbprep.hlp.h $(VBLIBS)
	$(CXX) -o vbprep vbprep.cpp $(LIBS) $(CXXFLAGS)

vbse: vbse.cpp vbse.hlp.h $(VBLIBS)
	$(CXX) -o vbse vbse.cpp $(LIBS) $(CXXFLAGS)

vbvec2hdr: vbvec2hdr.cpp vbvec2hdr.hlp.h $(VBLIBS)
	$(CXX) -o vbvec2hdr vbvec2hdr.cpp $(LIBS) $(CXXFLAGS)

vecsplit: vecsplit.cpp vecsplit.hlp.h $(VBLIBS)
	$(CXX) -o vecsplit vecsplit.cpp $(LIBS) $(CXXFLAGS)

test: test.cpp $(VBLIBS)
	$(CXX) -o test test.cpp $(LIBS) $(CXXFLAGS)
