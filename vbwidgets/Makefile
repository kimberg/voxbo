
# Makefile for vbwidgets

-include ../make_vars.txt
include ../make_stuff.txt

CXXFLAGS += -DQT3_SUPPORT $(QTINCDIRS) $(PICFLAG)

VBWIDGET_OBJECTS = fileview.o plotscreen.o qdecimalspinbox.o myboxes.o vbcontrast.o covariates.o\
		threshcalc.o runseq.o\
		moc_fileview.o moc_plotscreen.o moc_qdecimalspinbox.o moc_myboxes.o moc_vbcontrast.o\
		moc_covariates.o moc_threshcalc.o moc_runseq.o

# miscellaneous flags and such
XLIBS = $(QTLIBDIRS) -lX11 -lXext -lpthread
LIBS = $(LDFLAGS) $(LIBDIRS) $(XLIBS) -Xlinker -lvbprefs -lvbio -lvbutil -lz $(DLLIB) -lvbglm -lgsl -lgslcblas

ifdef VB_SHARED
  TARGETS=libvbwidgets.so
else
  TARGETS=libvbwidgets.a
endif

ifdef VB_NOX
  TARGETS=
endif

# the main targets

all: $(TARGETS)

install: $(TARGETS)
ifdef VB_SHARED
	cp -Pf $(TARGETS) $(VB_LIBDIR)
else
	@echo "Nothing to install here"
endif

clean:
	rm -f $(CLEANME) $(TARGETS)

$(VBLIBS):
	+make -C ../lib $@

libvbwidgets.a: $(VBWIDGET_OBJECTS)
	ar rc libvbwidgets.a $(VBWIDGET_OBJECTS)
	ranlib libvbwidgets.a

libvbwidgets.so: $(VBWIDGET_OBJECTS)
	g++ -shared -Wl,-soname,$@ $(LDFLAGS) -o $@ -lc $^ -L../lib -lvbio -lvbutil -lvbprefs -lvbglm -lQtCore -lQt3Support -lQtGui -lgsl -lboost_system

moc_%.cpp : %.h
	$(MOC) $< -o $@

%.o : %.cpp %.h
	$(CXX) -c $(CXXFLAGS) $*.cpp

moc_%.o: moc_%.cpp %.h
	$(CXX) -c $(CXXFLAGS) moc_$*.cpp

bbdialog.moc.h: bbdialog.h
	$(MOC) bbdialog.h -o bbdialog.moc.h	

bbdialog.o: bbdialog.h bbdialog.cpp
	$(CXX) -c $(CXXFLAGS) -o bbdialog.o bbdialog.cpp

# the following are not used currently

fileviewmain.o: fileviewmain.cpp 
	$(CXX) -c $(CXXFLAGS) fileviewmain.cpp

vbcontrast_standalone.o: vbcontrast_standalone.cpp vbcontrast.h covariates.h
	$(CXX) -c $(CXXFLAGS) vbcontrast_standalone.cpp

test: test.o plotscreen.o moc_plotscreen.o
	$(CXX) -o test test.o plotscreen.o moc_plotscreen.o $(LIBS)

test.o: test.cpp plotscreen.h 
	$(CXX) -c $(CXXFLAGS) test.cpp

