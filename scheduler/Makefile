
# Makefile for voxbo scheduler and related executables

-include ../make_vars.txt
include ../make_stuff.txt

LIBS=$(LDFLAGS) $(LIBDIRS) -lvbprefs -lvbutil -lz -lboost_system
ALLBINS=voxbo vbsrvd voxq vbq vq

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else
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
	rm -f $(CLEANME) $(ALLBINS)

$(VBLIBS):
	+make -C ../lib $@

# here are all the executables

voxbo: voxbo.o schedlib.o $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o voxbo voxbo.o schedlib.o $(LIBS)

vbsrvd: vbsrvd.o $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o vbsrvd vbsrvd.o $(LIBS)

voxq: voxq.o $(VBLIBS)
	$(CXX) $(CXXFLAGS) -o voxq voxq.o $(LIBS)

vbq: voxq
	ln -sf ./voxq vbq

vq: voxq
	ln -sf ./voxq vq

# and now for the building blocks

vbsrvd.o: vbsrvd.cpp
	$(CXX) $(CXXFLAGS) -c -o vbsrvd.o vbsrvd.cpp

voxq.o: voxq.cpp
	$(VBRC) voxq.hlp.h -help1 voxq1.hlp -help2 voxq2.hlp
	$(CXX) $(CXXFLAGS) -c -o voxq.o voxq.cpp

voxbo.o: voxbo.cpp
	$(CXX) $(CXXFLAGS) -c -o voxbo.o voxbo.cpp

schedlib.o: schedlib.cpp
	$(CXX) $(CXXFLAGS) -c -o schedlib.o schedlib.cpp
