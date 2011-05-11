
# makefile for brainbrowser

-include ../make_vars.txt
include ../make_stuff.txt

VBLIBS=$(VBLIBS2)

CXXFLAGS += -DQT3_SUPPORT $(QTLIBDIRS) $(QTINCDIRS)

LIBS =$(LDFLAGS) $(LIBDIRS) $(QTLIBDIRS) $(QTLIBS) $(DLLIB) -ldb_cxx
LIBOBJECTS=../lib/brain_tab.o ../lib/brain_util.o ../lib/mydb.o 
QTOBJECTS=../vbwidgets/bbdialog.o ../vbwidgets/myboxes.o 

ALLBINS=brainBrowser
BINS=brainBrowser

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

# executables

brainBrowser: main.o
	$(CXX) -o brainBrowser $(LIBOBJECTS) $(QTOBJECTS) main.o $(LIBS)

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) -o main.o main.cpp

searchBrain: searchBrain.o 
	$(CXX) --whole-archive -o searchBrain searchBrain.o $(LIBOBJECTS) -ldb_cxx -lpthread

searchBrain.o: searchBrain.cpp searchBrain.h
	$(CXX) -c -I../lib -Wall -W -O3 -o searchBrain.o searchBrain.cpp

