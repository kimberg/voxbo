
# Makefile for new scripting stuff

-include ../make_vars.txt
include ../make_stuff.txt
VBLIBS=$(VBLIBS3)

CXXFLAGS += -DQT3_SUPPORT $(QTINCDIRS)

O_SUBMITSEQUENCE = submit_sequence.o
O_DATASETVIEWER = display_dataset.o vbdatasetwidget.o moc_vbdatasetwidget.o
O_DATASETPRINTER = print_dataset.o
O_VBSEQUENCE = main.o mainwindow.o vbjobtypelistmodel.o vbsequenceview.o \
		vbsequencescene.o vbsequenceitem.o vbdatasetwidget.o moc_mainwindow.o \
		moc_vbjobtypelistmodel.o moc_vbsequenceview.o moc_vbsequencescene.o \
		moc_vbsequenceitem.o moc_vbdatasetwidget.o
XLIBS=$(QTLIBDIRS)
LIBS=$(LDFLAGS) $(LIBDIRS) $(XLIBS) -lvbprefs -lvbio -lvbutil -lvbscripts -lz -lpng $(DLLIB) $(GSL_LIBS)

ALLBINS=vbsequence submit_sequence print_dataset display_dataset

ifndef VB_NOX
  BINS=$(ALLBINS)
else
  BINS=submit_sequence
endif

ifneq ($(VB_TARGET),all)
	BINS=
endif

all: $(BINS)

install: $(BINS)
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f moc_* *.o *.exe $(ALLBINS)

$(VBLIBS):
	+make -C ../lib $@

# executables

submit_sequence: $(O_SUBMITSEQUENCE) $(VBLIBS)
	$(CXX) -o submit_sequence submit_sequence.o $(VBOBJECTS) $(OBJCOMP) $(LIBS)

print_dataset: $(O_DATASETPRINTER) $(VBLIBS)
	$(CXX) -o print_dataset $(O_DATASETPRINTER) $(LIBS) $(QTLIBS)

display_dataset: $(O_DATASETVIEWER) $(VBLIBS)
	$(CXX) -o display_dataset $(O_DATASETVIEWER) $(LIBS) $(QTLIBS)

vbsequence:  $(O_VBSEQUENCE) $(VBLIBS)
	$(CXX) -o vbsequence $(O_VBSEQUENCE) $(LIBS) $(QTLIBS)

# mocs

moc_mainwindow.cpp: mainwindow.h
	$(MOC) mainwindow.h -o moc_mainwindow.cpp

moc_vbjobtypelistmodel.cpp: vbjobtypelistmodel.h
	$(MOC) vbjobtypelistmodel.h -o moc_vbjobtypelistmodel.cpp

moc_vbsequenceview.cpp: vbsequenceview.h
	$(MOC) vbsequenceview.h -o moc_vbsequenceview.cpp

moc_vbsequencescene.cpp: vbsequencescene.h
	$(MOC) vbsequencescene.h -o moc_vbsequencescene.cpp

moc_vbsequenceitem.cpp: vbsequenceitem.h
	$(MOC) vbsequenceitem.h -o moc_vbsequenceitem.cpp

moc_vbdatasetwidget.cpp: vbdatasetwidget.h
	$(MOC) vbdatasetwidget.h -o moc_vbdatasetwidget.cpp

# objects

main.o: main.cpp mainwindow.h
	$(CXX) -c $(CXXFLAGS) main.cpp -o main.o

mainwindow.o: mainwindow.cpp mainwindow.h vbjobtypelistmodel.h vbsequenceview.h 
	$(CXX) -c $(CXXFLAGS) mainwindow.cpp -o mainwindow.o

vbjobtypelistmodel.o: vbjobtypelistmodel.cpp vbjobtypelistmodel.h
	$(CXX) -c $(CXXFLAGS) vbjobtypelistmodel.cpp -o vbjobtypelistmodel.o

vbsequenceview.o: vbsequenceview.cpp vbsequenceview.h vbsequencescene.h
	$(CXX) -c $(CXXFLAGS) vbsequenceview.cpp -o vbsequenceview.o

vbsequencescene.o: vbsequencescene.cpp vbsequencescene.h vbsequenceitem.h
	$(CXX) -c $(CXXFLAGS) vbsequencescene.cpp -o vbsequencescene.o

vbsequenceitem.o: vbsequenceitem.cpp vbsequenceitem.h vbsequencescene.h
	$(CXX) -c $(CXXFLAGS) vbsequenceitem.cpp -o vbsequenceitem.o

vbdatasetwidget.o: vbdatasetwidget.cpp vbdatasetwidget.h
	$(CXX) -c $(CXXFLAGS) vbdatasetwidget.cpp -o vbdatasetwidget.o

moc_mainwindow.o: moc_mainwindow.cpp
	$(CXX) -c $(CXXFLAGS) moc_mainwindow.cpp -o moc_mainwindow.o 

moc_vbjobtypelistmodel.o: moc_vbjobtypelistmodel.cpp 
	$(CXX) -c $(CXXFLAGS) moc_vbjobtypelistmodel.cpp -o moc_vbjobtypelistmodel.o

moc_vbsequenceview.o: moc_vbsequenceview.cpp 
	$(CXX) -c $(CXXFLAGS) moc_vbsequenceview.cpp -o moc_vbsequenceview.o

moc_vbsequencescene.o: moc_vbsequencescene.cpp 
	$(CXX) -c $(CXXFLAGS) moc_vbsequencescene.cpp -o moc_vbsequencescene.o

moc_vbsequenceitem.o: moc_vbsequenceitem.cpp 
	$(CXX) -c $(CXXFLAGS) moc_vbsequenceitem.cpp -o moc_vbsequenceitem.o

moc_vbdatasetwidget.o: moc_vbdatasetwidget.cpp
	$(CXX) -c $(CXXFLAGS) moc_vbdatasetwidget.cpp -o moc_vbdatasetwidget.o

print_dataset.o: print_dataset.cpp
	$(CXX) -c $(CXXFLAGS) print_dataset.cpp -o print_dataset.o

display_dataset.o: display_dataset.cpp
	$(CXX) -c $(CXXFLAGS) display_dataset.cpp -o display_dataset.o

