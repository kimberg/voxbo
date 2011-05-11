
# Makefile for vbview

include ../make_stuff.txt
-include ../make_vars.txt

VBLIBS=$(VBLIBS2)

CXXFLAGS += -DQT3_SUPPORT $(QTLIBDIRS) $(QTINCDIRS)

VBVIEW_SUBOBJECTS=vbview.o vbview_ts.o vbview_layers.o vbview_render.o vbview_io.o vbqt_masker.o vbqt_canvas.o vbqt_glmselect.o vbqt_scalewidget.o vbview_widgets.o rsrc.o
VBVIEW_OBJECTS=vbviewmain.o $(VBVIEW_SUBOBJECTS)

LIBS=$(LDFLAGS) $(LIBDIRS) $(QTLIBDIRS) -L../vbwidgets $(QTLIBS) -lvbglm -lvbprefs -lvbio -lvbutil -lz $(DLLIB) -lgsl -lgslcblas

# right now all three programs are in all packages, so we just
# conditionalize on ARCH

ALLBINS=vbview2 vbri

ifeq ($(ARCH),DARWIN)
  ALLBINS+=vbview2.app vbview2.dmg vbri.app vbri.dmg
endif

BINS=$(ALLBINS)

ifdef VB_NOX
  BINS=
endif

all: $(BINS)

install: $(BINS)
	cp -r $(BINS) $(VB_BINDIR)

clean:
	rm -rf $(CLEANME) $(ALLBINS)

# rule to build the libvoxbo libraries

$(VBLIBS):
	+make -C ../lib $@

%.hlp.h : %.hlp
	$(VBRC) $@ -myhelp $<

# executables

test: test.o
	$(CXX) -o test test.o $(LIBS)

test.o: test.cpp test.h
	$(MOC) test.h -o test.moc.h
	$(CXX) -c test.cpp $(CXXFLAGS)

vbview2: $(VBVIEW_OBJECTS) $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbview2 $(VBVIEW_OBJECTS) $(LIBS)

vbview2.app: vbview2
	mkdir -p vbview2.app/Contents/MacOS
	echo "APPL????" > vbview2.app/Contents/PkgInfo
	cp vbview2 vbview2.app/Contents/MacOS

vbview2.dmg: vbview2 vbview2.app/Contents/PkgInfo
	@rm -f vbview2.dmg
	hdiutil create -fs HFS+ -volname vbview2 -srcfolder vbview2.app vbview2.dmg

vbrender: vb_render.o vbrender.o $(VBLIBS)
	$(CXX) -o vbrender vbrender.o vb_render.o $(LIBS)

vbri: vbri.cpp vbri.moc.h $(VBVIEW_SUBOBJECTS)
	$(CXX) $(CXXFLAGS) -o vbri vbri.cpp $(VBVIEW_SUBOBJECTS) $(LIBS)

vbri.app: vbri
	mkdir -p vbri.app/Contents/MacOS
	echo "APPL????" > vbri.app/Contents/PkgInfo
	cp vbri vbri.app/Contents/MacOS

vbri.dmg: vbri vbri.app/Contents/PkgInfo
	@rm -f vbri.dmg
	hdiutil create -fs HFS+ -volname vbri -srcfolder vbri.app vbri.dmg

# the objects for vbrender

vbrender.o: vbrender.cpp
	$(CXX) -c vbrender.cpp $(CXXFLAGS)

vb_render.o: vb_render.cpp
	$(CXX) -c vb_render.cpp $(CXXFLAGS)

# the objects for vbview

vbview.o: vbview.moc.h vbview.cpp
	$(CXX) -c vbview.cpp $(CXXFLAGS)

vbviewmain.o: vbviewmain.moc.h vbviewmain.cpp vbview.hlp.h vbview.h
	$(CXX) -c vbviewmain.cpp $(CXXFLAGS)

vbview_ts.o: vbview_ts.cpp vbview.h
	$(CXX) -c vbview_ts.cpp $(CXXFLAGS)

vbview_io.o: vbview_io.cpp vbview.h
	$(CXX) -c vbview_io.cpp $(CXXFLAGS)

vbview_layers.o: vbview_layers.cpp vbview.h
	$(CXX) -c vbview_layers.cpp $(CXXFLAGS)

vbview_render.o: vbview_render.cpp vbview.h
	$(CXX) -c vbview_render.cpp $(CXXFLAGS)

rsrc.o : rsrc.qrc
	$(RCC) rsrc.qrc -o rsrc.cpp
	$(CXX) -c rsrc.cpp $(CXXFLAGS)

# custom qt widgets

vbview_widgets.o: vbview_widgets.moc.h vbview_widgets.cpp
	$(CXX) -c vbview_widgets.cpp $(CXXFLAGS)

vbqt_canvas.o: vbqt_canvas.moc.h vbqt_canvas.cpp
	$(CXX) -c vbqt_canvas.cpp $(CXXFLAGS)

vbqt_masker.o: vbqt_masker.moc.h vbqt_masker.cpp
	$(CXX) -c vbqt_masker.cpp $(CXXFLAGS)

vbqt_glmselect.o: vbqt_glmselect.moc.h vbqt_glmselect.cpp
	$(CXX) -c vbqt_glmselect.cpp $(CXXFLAGS)

vbqt_scalewidget.o: vbqt_scalewidget.moc.h vbqt_scalewidget.cpp
	$(CXX) -c vbqt_scalewidget.cpp $(CXXFLAGS)

# moc files

%.moc.h : %.h
	$(MOC) $< -o $@

# dummy target for vbview

vbview:
	@echo "Please make vbview2 instead!"
