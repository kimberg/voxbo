
# Makefile in util directory

-include ../make_vars.txt
include ../make_stuff.txt

LIBS=$(LDFLAGS) $(LIBDIRS) -lm -ldbutil -lgnutls -lgnutls-extra -lgcrypt -lgpg-error -ldb_cxx -lvbprefs -lvbglm -lvbio -lvbutil -lgsl -lgslcblas -lz -lpng -ldl -lpthread

ALLBINS=initDB initScoreValues
# DYK: these binaries are not needed; this kind of functionality
# should be in client/dbtool anyway
ALLBINS=

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),db)
	BINS=$(ALLBINS)
else
	BINS=
endif

# main targets

all: $(BINS)

install:
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(ALLBINS)

initScoreNames: initScoreNames.o 
	$(CXX) -o initScoreNames initScoreNames.o $(LIBS)

initScoreValues: initScoreValues.o 
	$(CXX) -o initScoreValues initScoreValues.o $(LIBS)

initPerm: initPerm.o 
	$(CXX) -o initPerm initPerm.o $(LIBS)

showScoreNames: showScoreNames.o
	$(CXX) -o showScoreNames showScoreNames.o $(LIBS)

showPerm: showPerm.o
	$(CXX) -o showPerm showPerm.o $(LIBS)

showSessions: showSessions.o
	$(CXX) -o showSessions showSessions.o $(LIBS)

showScoreValues: showScoreValues.o
	$(CXX) -o showScoreValues showScoreValues.o $(LIBS)

showPatientScores: showPatientScores.o
	$(CXX) -o showPatientScores showPatientScores.o $(LIBS)

showUsers: showUsers.o
	$(CXX) -o showUsers showUsers.o $(LIBS)

showSystem: showSystem.o
	$(CXX) -o showSystem showSystem.o $(LIBS)

showRegionNames: showRegionNames.o
	$(CXX) -o showRegionNames showRegionNames.o $(LIBS)

showSynonyms: showSynonyms.o
	$(CXX) -o showSynonyms showSynonyms.o $(LIBS)

showRegionRelations: showRegionRelations.o
	$(CXX) -o showRegionRelations showRegionRelations.o $(LIBS)

showPatients: showPatients.o
	$(CXX) -o showPatients showPatients.o $(LIBS)

delRecord: delRecord.o 
	g++ -o delRecord delRecord.o $(LIBS)

cpDB:	cpDB.o 
	$(CXX) -o cpDB cpDB.o $(LIBS)

mvDB:	mvDB.o 
	$(CXX) -o mvDB mvDB.o $(LIBS)

addPerm: addPerm.o 
	$(CXX) -o addPerm addPerm.o $(LIBS)

delPerm: delPerm.o 
	$(CXX) -o delPerm delPerm.o $(LIBS)

resetEnv: resetEnv.o
	$(CXX) -o resetEnv resetEnv.o $(LIBS)

addPatientList: addPatientList.o
	$(CXX) -o addPatientList addPatientList.o $(LIBS)

showPatientList: showPatientList.o
	$(CXX) -o showPatientList showPatientList.o $(LIBS)

resetSystem: resetSystem.o
	$(CXX) -o resetSystem resetSystem.o $(LIBS)

initSystem: initSystem.o
	$(CXX) -o initSystem initSystem.o $(LIBS)

initDB:	initDB.o 
	$(CXX) -o initDB initDB.o $(LIBS)

# object file generation
initScoreNames.o: initScoreNames.cpp
	$(CXX) -c initScoreNames.cpp $(CXXFLAGS) 

initScoreValues.o: initScoreValues.cpp
	$(CXX) -c initScoreValues.cpp $(CXXFLAGS) 	

initPerm.o: initPerm.cpp
	$(CXX) -c initPerm.cpp $(CXXFLAGS) 

showScoreNames.o: showScoreNames.cpp
	$(CXX) -c showScoreNames.cpp $(CXXFLAGS) 

showPerm.o: showPerm.cpp
	$(CXX) -c showPerm.cpp $(CXXFLAGS) 

showSessions.o: showSessions.cpp
	$(CXX) -c showSessions.cpp $(CXXFLAGS) 

showScoreValues.o: showScoreValues.cpp
	$(CXX) -c showScoreValues.cpp $(CXXFLAGS) 

showPatientScores.o: showPatientScores.cpp
	$(CXX) -c showPatientScores.cpp $(CXXFLAGS) 

showUsers.o: showUsers.cpp
	$(CXX) -c showUsers.cpp $(CXXFLAGS) 

showSystem.o: showSystem.cpp
	$(CXX) -c showSystem.cpp $(CXXFLAGS)
 
resetSystem.o: resetSystem.cpp
	$(CXX) -c resetSystem.cpp $(CXXFLAGS) 

initSystem.o: initSystem.cpp
	$(CXX) -c initSystem.cpp $(CXXFLAGS) 

showRegionNames.o: showRegionNames.cpp
	$(CXX) -c showRegionNames.cpp $(CXXFLAGS) 

showSynonyms.o: showSynonyms.cpp
	$(CXX) -c showSynonyms.cpp $(CXXFLAGS) 

showRegionRelations.o: showRegionRelations.cpp
	$(CXX) -c showRegionRelations.cpp $(CXXFLAGS) 

showPatients.o: showPatients.cpp
	$(CXX) -c showPatients.cpp $(CXXFLAGS) 

initDB.o: initDB.cpp
	g++ -c initDB.cpp $(CXXFLAGS) 

delRecord.o: delRecord.cpp
	g++ -c delRecord.cpp

cpDB.o: cpDB.cpp
	g++ -c cpDB.cpp

mvDB.o: mvDB.cpp
	g++ -c mvDB.cpp

addPerm.o: addPerm.cpp
	g++ -c addPerm.cpp

delPerm.o: delPerm.cpp
	g++ -c delPerm.cpp

resetEnv.o: resetEnv.cpp
	g++ -c resetEnv.cpp

addPatientList.o: addPatientList.cpp
	g++ -c addPatientList.cpp $(CXXFLAGS)
 
showPatientList.o: showPatientList.cpp
	g++ -c showPatientList.cpp $(CXXFLAGS) 



