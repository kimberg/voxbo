
CPLUS_INCLUDE_PATH=/usr/local/include:/sw/include
LIBRARY_PATH=/usr/local/lib:/sw/lib
export CPLUS_INCLUDE_PATH LIBRARY_PATH


OBJS=connect.o tokenlist.o vbutil.o dbserver.o
LIBS=$(LDFLAGS) -lgnutls-extra -lgnutls -lgcrypt -lz

dbserver: $(OBJS)
	g++ -o dbserver $(OBJS) $(LIBS)

connect.o: vbutil.h connect.cpp
	g++ -c connect.cpp

tokenlist.o: tokenlist.h tokenlist.cpp
	g++ -c tokenlist.cpp

vbutil.o: vbutil.h vbutil.cpp
	g++ -c vbutil.cpp

dbserver.o: vbutil.h dbserver.cpp
	g++ -c dbserver.cpp -Wall

clean:
	rm -f *.o *~ dbserver
