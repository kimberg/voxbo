
# Makefile for database client

-include ../make_vars.txt
include ../make_stuff.txt

CXXFLAGS += -DQT3_SUPPORT $(QTINCDIRS) -I../vbview

VBVIEW_OBJECTS=../vbview/vbview.o ../vbview/vbview_ts.o ../vbview/vbview_layers.o ../vbview/vbview_render.o ../vbview/vbview_io.o ../vbview/vbqt_masker.o ../vbview/vbqt_canvas.o ../vbview/vbqt_glmselect.o ../vbview/vbqt_scalewidget.o ../vbview/vbview_widgets.o
XLIBS=$(LDFLAGS) $(LIBDIRS) $(QTLIBDIRS)  $(VBVIEW_OBJECTS) $(QTLIBS) -lvbglm -lvbprefs -lvbio -lvbutil -lz -lm -lpng -ldl -lgnutls-extra -lgnutls -lgcrypt -lgpg-error -ldbutil -lgsl -lgslcblas -ldb_cxx
NOXLIBS=$(LDFLAGS) $(LIBDIRS) -lvbglm -lvbprefs -lvbio -lvbutil -lz -lm -lpng -ldl -lgnutls-extra -lgnutls -lgcrypt -lgpg-error -ldbutil -lgsl -lgslcblas -ldb_cxx -lpthread

W_OBJECTS=dbqscorebox.o dbqlineedit.o dbqimage.o dbqcombo.o dbqtimedate.o dbqtextedit.o dbqstub.o dbview.o dbclient.o dbqbrain.o dbdialogs.o dbmainwindow.o

ALLBINS=client dbtool

ifeq ($(VB_TARGET),all)
	BINS=$(ALLBINS)
else ifeq ($(VB_TARGET),db)
	BINS=$(ALLBINS)
else
	BINS=
endif

ifdef VB_NOX
  BINS=
endif

all: $(BINS)

install:
	echo "Not yet!"

clean:
	rm -rf $(CLEANME) $(ALLBINS) dbfiles.h

# executables

client : $(W_OBJECTS) client.cpp rsrc.cpp dbclient.h $(VBLIBSDB)
	$(CXX) $(CXXFLAGS) client.cpp rsrc.cpp $(W_OBJECTS) -o client $(XLIBS)

dbtool : dbtool.cpp dbclient.o dbfiles.h $(VBLIBSDB)
	$(CXX) $(CXXFLAGS) dbtool.cpp dbclient.o -o dbtool $(NOXLIBS) -lreadline -ltermcap

test : $(W_OBJECTS) test.cpp rsrc.cpp $(VBLIBSDB)
	$(CXX) $(CXXFLAGS) test.cpp rsrc.cpp $(W_OBJECTS) -o test $(XLIBS)

# all my pattern rules

%.moc.h : %.h
	$(MOC) $< -o $@

rsrc.cpp : client.qrc
	$(RCC) client.qrc -o rsrc.cpp

dbfiles.h : default_scorenames.txt default_types.txt default_views.txt\
			test_scorenames.txt test_types.txt test_views.txt
	$(VBRC) dbfiles.h def_sn default_scorenames.txt def_ty default_types.txt def_vi default_views.txt\
			test_sn test_scorenames.txt test_ty test_types.txt test_vi test_views.txt

dbmainwindow.o : dbmainwindow.cpp dbmainwindow.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbdialogs.o : dbdialogs.cpp dbdialogs.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqlineedit.o : dbqlineedit.cpp dbqlineedit.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqimage.o : dbqimage.cpp dbqimage.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqbrain.o : dbqbrain.cpp dbqbrain.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqtextedit.o : dbqtextedit.cpp dbqtextedit.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqcombo.o : dbqcombo.cpp dbqcombo.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqscorebox.o : dbqscorebox.cpp dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqstub.o : dbqstub.cpp dbqstub.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbqtimedate.o : dbqtimedate.cpp dbqtimedate.moc.h dbqscorebox.moc.h
	$(CXX) -c $< $(CXXFLAGS)

dbview.o : dbview.cpp dbview.moc.h
	$(CXX) -c $< $(CXXFLAGS)

moc_searchPatient.cpp: searchPatient.h
	$(MOC) $(DEFINES) $(INCPATH) searchPatient.h -o moc_searchPatient.cpp

searchPatient.o: searchPatient.cpp searchPatient.h
	$(CXX) -c $< $(CXXFLAGS)

dbclient.o: dbclient.cpp dbclient.h
	$(CXX) -c $< $(CXXFLAGS)

moc_searchPatient.o: moc_searchPatient.cpp 
	$(CXX) -c $< $(CXXFLAGS)

#connx2srv.o: connx2srv.cpp
#	$(CXX) -c $< $(CXXFLAGS)

