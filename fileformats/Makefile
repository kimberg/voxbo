
# Makefile for fileformats

-include ../make_vars.txt
include ../make_stuff.txt

SHAREDFLAG=-shared

LIBS=$(LDFLAGS) -Wl,--no-as-needed $(LIBDIRS) -lvbio -lvbutil -lvbprefs -lz -lpng -lgsl -lgslcblas -lboost_system

ALLBINS=dcmsplit dicominfo ffinfo vbrename analyzeinfo niftiinfo
BINS=$(ALLBINS)

# built-in types are not built here by default

all: $(BINS)

install:
ifdef BINS
	cp -Pf $(BINS) $(VB_BINDIR)
else
	@echo "Nothing to install."
endif

clean:
	rm -f $(CLEANME) $(ALLBINS)

# rule to build the libvoxbo libraries

$(VBLIBS):
	+make -C ../lib $@

%.hlp.h : %.hlp
	$(VBRC) $@ -myhelp $<

ffinfo: ffinfo.cpp $(VBLIBS)
	$(CXX) ffinfo.cpp $(DLLIB) -o ffinfo $(LIBS) $(CXXFLAGS)

dicominfo: dicominfo.cpp $(VBLIBS)
	$(CXX) dicominfo.cpp -o dicominfo $(LIBS) $(CXXFLAGS)

analyzeinfo: analyzeinfo.cpp $(VBLIBS)
	$(CXX) analyzeinfo.cpp -o analyzeinfo $(LIBS) $(CXXFLAGS)

niftiinfo: analyzeinfo
	ln -sf ./analyzeinfo niftiinfo

vbrename: vbrename.cpp vbrename.hlp.h $(VBLIBS)
	$(CXX) vbrename.cpp -o vbrename $(LIBS) $(CXXFLAGS)

dcmsplit: dcmsplit.cpp dcmsplit.hlp.h $(VBLIBS)
	$(CXX) dcmsplit.cpp -o dcmsplit $(LIBS) $(CXXFLAGS)

# types

ff_cub.o: ff_cub.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_cub.cpp -o ff_cub.o $(CXXFLAGS)

ff_tes.o: ff_tes.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_tes.cpp -o ff_tes.o $(CXXFLAGS)

ff_ref.o: ff_ref.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_ref.cpp -o ff_ref.o $(CXXFLAGS)

ff_ge.o: ff_ge.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_ge.cpp -o ff_ge.o $(CXXFLAGS)

ff_dicom3d.o: ff_dicom3d.cpp dicom.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_dicom3d.cpp -o ff_dicom3d.o dicom.o $(CXXFLAGS) # -z muldefs

ff_dicom4d.o: ff_dicom4d.cpp dicom.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_dicom4d.cpp -o ff_dicom4d.o dicom.o $(CXXFLAGS)

ff_analyze3d.o: ff_analyze3d.cpp analyze.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_analyze3d.cpp -o ff_analyze3d.o analyze.o $(CXXFLAGS)

ff_analyze4d.o: ff_analyze4d.cpp analyze.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_analyze4d.cpp -o ff_analyze4d.o analyze.o $(CXXFLAGS)

ff_nifti3d.o: ff_nifti3d.cpp nifti.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_nifti3d.cpp -o ff_nifti3d.o nifti.o $(CXXFLAGS)

ff_nifti4d.o: ff_nifti4d.cpp nifti.o $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_nifti4d.cpp -o ff_nifti4d.o nifti.o $(CXXFLAGS)

ff_da.o: ff_da.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_da.cpp -o ff_da.o $(CXXFLAGS)

ff_vmp3d.o: ff_vmp3d.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_vmp3d.cpp -o ff_vmp3d.o $(CXXFLAGS)

ff_xxx.o: ff_xxx.cpp $(VBLIBS)
	$(CXX) $(SHAREDFLAG) ff_xxx.cpp -o ff_xxx.o $(CXXFLAGS)

# support for types

dicom.o: dicom.cpp dicom.h
	$(CXX) -c dicom.cpp $(CXXFLAGS)

analyze.o: analyze.cpp analyze.h
	$(CXX) -c analyze.cpp $(CXXFLAGS)

nifti.o: nifti.cpp nifti.h nifti11_ref.h
	$(CXX) -c nifti.cpp $(CXXFLAGS)
