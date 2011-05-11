
# Makefile for building: glm, gdw, vecview

include ../make_stuff.txt
-include ../make_vars.txt

CXXFLAGS += -DQT3_SUPPORT $(QTINCDIRS) -I../utils

PERM_EX=../stand_alone/perm.o ../stand_alone/utils.o ../stand_alone/time_series_avg.o
GDW_EX=../utils/fitOneOverF.o

# define object variables
GLM_OBJECTS=glm_main.o glm.o gdw.o gheaderinfo.o block.o paired.o glm_tab3.o
GLM_OBJMOC = moc_glm.o moc_gdw.o moc_block.o moc_paired.o moc_glm_tab3.o
GDW_OBJECTS=gdw_main.o gdw.o gheaderinfo.o block.o paired.o
GDW_OBJMOC = moc_gdw.o moc_block.o moc_paired.o
VECVIEW_OBJECTS = vecview.o
VECVIEW_OBJMOC = moc_vecview.o
TCALC_OBJECTS= vbtcalc.o
PERMGEN_OBJECTS= vbpermgen.o

# miscellaneous flags and such
LIBS=$(LDFLAGS) $(LIBDIRS) $(QTLIBDIRS) $(QTLIBS) -lz -lvbglm -lvbprefs -lvbio -lvbutil -lz $(GSL_LIBS) $(DLLIB)

ALLBINS= glm gdw vecview vbpermgen vbtcalc
OSXTRA=glm.app glm.dmg gdw.app gdw.dmg vecview.app vecview.dmg vbpermgen.app vbpermgen.dmg vbtcalc.app vbtcalc.dmg

ifeq ($(ARCH),DARWIN)
  ALLBINS+=$(OSXTRA)
endif

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),spm)
	BINS=$(ALLBINS)
else
	BINS=
endif

ifdef VB_NOX
  BINS=
endif

# the main targets
all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -rf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -rf $(CLEANME) $(ALLBINS)

$(VBLIBS):
	+make -C ../lib $@

%.hlp.h : %.hlp
	$(VBRC) $@ -myhelp $<

%.moc.h : %.h
	$(MOC) $< -o $@

glm: $(GLM_OBJECTS) $(GLM_OBJMOC) $(VBLIBS) $(GDW_EX)
	$(CXX) -o glm $(GLM_OBJECTS) $(GLM_OBJMOC) $(GDW_EX) $(LIBS)

glm.app: glm
	mkdir -p glm.app/Contents/MacOS
	echo "APPL????" > glm.app/Contents/PkgInfo
	cp glm glm.app/Contents/MacOS

glm.dmg: glm glm.app/Contents/PkgInfo
	@rm -f glm.dmg
	hdiutil create -fs HFS+ -volname glm -srcfolder glm.app glm.dmg

gdw: $(GDW_OBJECTS) $(GDW_OBJMOC) $(VBLIBS) $(GDW_EX)
	$(CXX) -o gdw $(GDW_OBJECTS) $(GDW_OBJMOC) $(GDW_EX) $(LIBS)

gdw.app: gdw
	mkdir -p gdw.app/Contents/MacOS
	echo "APPL????" > gdw.app/Contents/PkgInfo
	cp gdw gdw.app/Contents/MacOS

gdw.dmg: gdw gdw.app/Contents/PkgInfo
	@rm -f gdw.dmg
	hdiutil create -fs HFS+ -volname gdw -srcfolder gdw.app gdw.dmg

vecview: $(VECVIEW_OBJECTS) $(VECVIEW_OBJMOC) $(VBLIBS) $(GDW_EX)
	$(CXX) -o vecview $(VECVIEW_OBJECTS) $(VECVIEW_OBJMOC) $(GDW_EX) $(LIBS)

vecview.app: vecview
	mkdir -p vecview.app/Contents/MacOS
	echo "APPL????" > vecview.app/Contents/PkgInfo
	cp vecview vecview.app/Contents/MacOS

vecview.dmg: vecview vecview.app/Contents/PkgInfo
	@rm -f vecview.dmg
	hdiutil create -fs HFS+ -volname vecview -srcfolder vecview.app vecview.dmg

# thresholdcalculator executable
vbtcalc: $(TCALC_OBJECTS) $(VBLIBS)
	$(CXX) -o vbtcalc $(TCALC_OBJECTS) $(LIBS)

vbtcalc.app: vbtcalc
	mkdir -p vbtcalc.app/Contents/MacOS
	echo "APPL????" > vbtcalc.app/Contents/PkgInfo
	cp vbtcalc vbtcalc.app/Contents/MacOS

vbtcalc.dmg: vbtcalc vbtcalc.app/Contents/PkgInfo
	@rm -f vbtcalc.dmg
	hdiutil create -fs HFS+ -volname vbtcalc -srcfolder vbtcalc.app vbtcalc.dmg

# vbpermgen executable
vbpermgen: $(PERMGEN_OBJECTS) $(PERM_EX) $(VBLIBS)
	$(CXX) -o vbpermgen $(PERMGEN_OBJECTS) $(PERM_EX) $(LIBS)

vbpermgen.app: vbpermgen
	mkdir -p vbpermgen.app/Contents/MacOS
	echo "APPL????" > vbpermgen.app/Contents/PkgInfo
	cp vbpermgen vbpermgen.app/Contents/MacOS

vbpermgen.dmg: vbpermgen vbpermgen.app/Contents/PkgInfo
	@rm -f vbpermgen.dmg
	hdiutil create -fs HFS+ -volname vbpermgen -srcfolder vbpermgen.app vbpermgen.dmg

# DEPENDENCIES ELSEWHERE

$(GDW_EX) $(PERM_EX):
	+make -C $(@D) $(@F)

# the local objects
glm_main.o: glm_main.cpp glm.h glm.hlp.h
	$(CXX) -c $(CXXFLAGS) glm_main.cpp

gdw_main.o: gdw_main.cpp gdw.h gdw.hlp.h
	$(CXX) -c $(CXXFLAGS) gdw_main.cpp

glm.o: glm.cpp glm.h gdw.h
	$(CXX) -c $(CXXFLAGS) glm.cpp

moc_glm.o: moc_glm.cpp glm.h 
	$(CXX) -c $(CXXFLAGS) moc_glm.cpp

moc_glm.cpp: $(MOC) glm.h
	$(MOC) glm.h -o moc_glm.cpp

gdw.o: gdw.cpp gdw.h ../vbwidgets/plotscreen.h gheaderinfo.h block.h paired.h
	$(CXX) -c $(CXXFLAGS) gdw.cpp

moc_gdw.o: moc_gdw.cpp gdw.h
	$(CXX) -c $(CXXFLAGS) moc_gdw.cpp

moc_gdw.cpp: $(MOC) gdw.h
	$(MOC) gdw.h -o moc_gdw.cpp

block.o: block.cpp block.h
	$(CXX) -c $(CXXFLAGS) block.cpp

moc_block.o: moc_block.cpp block.h
	$(CXX) -c $(CXXFLAGS) moc_block.cpp

moc_block.cpp: $(MOC) block.h
	$(MOC) block.h -o moc_block.cpp

paired.o: paired.cpp paired.h
	$(CXX) -c $(CXXFLAGS) paired.cpp

moc_paired.o: moc_paired.cpp paired.h
	$(CXX) -c $(CXXFLAGS) moc_paired.cpp

moc_paired.cpp: $(MOC) paired.h
	$(MOC) paired.h -o moc_paired.cpp

glm_tab3.o: glm_tab3.cpp glm_tab3.h
	$(CXX) -c $(CXXFLAGS) glm_tab3.cpp

moc_glm_tab3.o: moc_glm_tab3.cpp glm_tab3.h
	$(CXX) -c $(CXXFLAGS) moc_glm_tab3.cpp

moc_glm_tab3.cpp: $(MOC) glm_tab3.h
	$(MOC) glm_tab3.h -o moc_glm_tab3.cpp

gheaderinfo.o: gheaderinfo.cpp gheaderinfo.h
	$(CXX) -c $(CXXFLAGS) gheaderinfo.cpp

vecview.o: vecview.cpp vecview.h vecview.hlp.h ../vbwidgets/plotscreen.h 
	$(CXX) -c $(CXXFLAGS) vecview.cpp

moc_vecview.o: moc_vecview.cpp vecview.h
	$(CXX) -c $(CXXFLAGS) moc_vecview.cpp	

moc_vecview.cpp: $(MOC) vecview.h vecview.cpp 
	$(MOC) vecview.h -o moc_vecview.cpp

vbtcalc.o: vbtcalc.cpp vbtcalc.hlp.h
	$(CXX) -c $(CXXFLAGS) vbtcalc.cpp

vbpermgen.o: vbpermgen.cpp vbpermgen.moc.h vbpermgen.hlp.h
	$(CXX) -c $(CXXFLAGS) vbpermgen.cpp
