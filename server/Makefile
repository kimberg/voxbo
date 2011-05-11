
# Makefile for database server

-include ../make_vars.txt
include ../make_stuff.txt

LIBS=$(LDFLAGS) $(LIBDIRS) $(VBVIEW_OBJECTS) -lvbprefs -lvbglm -lvbio -lvbutil -lz -lm -lpng -ldl -lgnutls-extra -lgnutls -lgcrypt -lgpg-error -ldbutil -lgsl -lgslcblas -ldb_cxx -lpthread

ALLBINS=server

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),db)
	BINS=$(ALLBINS)
else
	BINS=
endif

# we're linking against qt, perhaps unnecessarily, so no server
ifdef VB_NOX
  BINS=
endif

all: $(BINS)

install:
	echo "No install in server dir right now."

clean:
	rm -f $(CLEANME) $(ALLBINS)

server: server.o
	$(CXX) $(CXXFLAGS) -o server server.o $(LIBS)

server.o: server.h server.cpp
	$(CXX) $(CXXFLAGS) -c server.cpp -Wall
