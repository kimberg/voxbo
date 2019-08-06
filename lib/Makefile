
# Makefile for libvoxbo

-include ../make_vars.txt
include ../make_stuff.txt

CXXFLAGS+=$(PICFLAG)

LIBS =$(LIBDIRS) $(LIBPATHS) -lm -lvbglm -lvbprefs -lvbio -lvbutil -lz $(DLLIB) $(GSL_LIBS)
IOOBJECTS=vbio.o tes.o cube.o imageutils.o mat.o png.o vb_vector.o\
          vbpreplib.o
FFOBJECTS=vbff.o ff_cub.o ff_tes.o ff_ref.o ff_dicom3d.o ff_dicom4d.o dicom.o\
          ff_img3d.o ff_img4d.o ff_imgdir.o\
          ff_nifti3d.o ff_nifti4d.o nifti.o\
          ff_roi.o analyze.o ff_ge.o ff_vmp3d.o ff_mat.o
UTILOBJECTS=vbutil.o endian.o connect.o tokenlist.o vbreports.o genericexcep.o
SCRIPTOBJECTS=vbscripttools.o vbsequence.o vbdataset.o vbexecdef.o
VBPOBJECTS=vbjobspec.o vbprefs.o vbhost.o vbx.o
GLMOBJECTS=makestatcub.o glmutil.o glm_stats.o statthreshold.o regress1.o trialavg.o stats.o

ifdef VB_SHARED
  EXT=so
else
  EXT=a
endif

TARGETS=libvbutil.$(EXT) libvbprefs.$(EXT) libvbio.$(EXT) libvbglm.$(EXT)

#ifdef VB_SHARED
#  TARGETS=libvbutil.so libvbprefs.so libvbio.so libvbglm.so
#else
#  TARGETS=libvbutil.a libvbprefs.a libvbio.a libvbglm.a
#endif

all: $(TARGETS)

clean:
	rm -f $(CLEANME) test

install: $(TARGETS)
ifdef VB_SHARED
	cp -Pf $(TARGETS) $(VB_LIBDIR)
else
	@echo "Nothing to install here"
endif

# the below can probably be made generic, but i'm too chicken to mess with it
# using $^ means the prereqs must be the objects

libvbio.a: $(IOOBJECTS) $(FFOBJECTS)
	ar rc libvbio.a $^
	ranlib libvbio.a

libvbio.so: $(IOOBJECTS) $(FFOBJECTS) libvbprefs.so libvbutil.so
	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) -L. $^ -lc -lz -lgsl -lpng -lvbprefs -lvbutil -lboost_system

libvbutil.a: $(UTILOBJECTS)
	ar rc libvbutil.a $^
	ranlib libvbutil.a

libvbutil.so: $(UTILOBJECTS)
	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) $^ -lc -lz -lboost_system

libvbprefs.a: $(VBPOBJECTS)
	ar rc libvbprefs.a $^
	ranlib libvbprefs.a

libvbprefs.so: $(VBPOBJECTS) libvbutil.so
	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) -L. $^ -lc -lvbutil -lboost_system

libvbscripts.a: $(SCRIPTOBJECTS)
	ar rc libvbscripts.a $^
	ranlib libvbscripts.a

libvbscripts.so: $(SCRIPTOBJECTS) libvbprefs.so libvbutil.so
	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) -L. $^ -lc -lvbprefs -lvbutil

libvbglm.a: $(GLMOBJECTS)
	ar rc libvbglm.a $^
	ranlib libvbglm.a

libvbglm.so: $(GLMOBJECTS) libvbio.so libvbutil.so libvbprefs.so
#	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) -L. $^ -lc -lgsl -lvbio -lvbutil -lvbscripts -lvbprefs
	g++ -shared -Wl,-soname,$@ -o $@ $(LDFLAGS) -L. $^ -lc -lgsl -lvbio -lvbutil -lvbprefs -lboost_system

# and now for the building blocks

vbscripttools.o: vbscripttools.cpp vbscripttools.h
	$(CXX) $(CXXFLAGS) -c vbscripttools.cpp

vbexecdef.o: vbexecdef.cpp vbexecdef.h vbscripttools.h vbsequence.h
	$(CXX) $(CXXFLAGS) -c vbexecdef.cpp

vbsequence.o: vbsequence.cpp vbsequence.h vbexecdef.h vbscripttools.h
	$(CXX) $(CXXFLAGS) -c vbsequence.cpp

vbdataset.o: vbdataset.cpp vbdataset.h vbscripttools.h
	$(CXX) $(CXXFLAGS) -c vbdataset.cpp

vbutil.o: vbutil.cpp vbutil.h
	$(CXX) $(CXXFLAGS) -c vbutil.cpp

tokenlist.o: tokenlist.cpp tokenlist.h
	$(CXX) $(CXXFLAGS) -c tokenlist.cpp

# stringtokenizer.o: stringtokenizer.cpp stringtokenizer.h
# 	$(CXX) $(CXXFLAGS) -c stringtokenizer.cpp

genericexcep.o: genericexcep.cpp genericexcep.h
	$(CXX) $(CXXFLAGS) -c genericexcep.cpp

endian.o: endian.cpp vbutil.h
	$(CXX) $(CXXFLAGS) -c endian.cpp

connect.o: connect.cpp vbutil.h
	$(CXX) $(CXXFLAGS) -c connect.cpp

vbjobspec.o: vbjobspec.cpp vbjobspec.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbjobspec.cpp

vbprefs.o: vbprefs.cpp vbprefs.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbprefs.cpp

vbhost.o: vbhost.cpp vbprefs.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbhost.cpp

vbx.o: vbx.cpp vbx.h
	$(CXX) $(CXXFLAGS) -c vbx.cpp

vbreports.o: vbreports.cpp vbprefs.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbreports.cpp

vbpreplib.o: vbpreplib.cpp vbpreplib.h vbprefs.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbpreplib.cpp

makestatcub.o: makestatcub.cpp makestatcub.h
	$(CXX) $(CXXFLAGS) -c makestatcub.cpp

glmutil.o: glmutil.cpp glmutil.h
	$(CXX) $(CXXFLAGS) -c glmutil.cpp

stats.o: stats.cpp stats.h vbutil.h
	$(CXX) $(CXXFLAGS) -c stats.cpp

glm_stats.o: glm_stats.cpp glmutil.h
	$(CXX) $(CXXFLAGS) -c glm_stats.cpp

regress1.o: regress1.cpp regress1.h
	$(CXX) $(CXXFLAGS) -c regress1.cpp

statthreshold.o: statthreshold.cpp 
	$(CXX) $(CXXFLAGS) -c statthreshold.cpp

trialavg.o: trialavg.cpp
	$(CXX) $(CXXFLAGS) -c trialavg.cpp

# vbff and the build-in fileformats

vbff.o: vbff.cpp vbio.h vbutil.h
	$(CXX) $(CXXFLAGS) -c vbff.cpp

ff_cub.o: ff_cub.cpp
	$(CXX) $(CXXFLAGS) -c ff_cub.cpp

ff_mat.o: ff_mat.cpp
	$(CXX) $(CXXFLAGS) -c ff_mat.cpp

ff_tes.o: ff_tes.cpp
	$(CXX) $(CXXFLAGS) -c ff_tes.cpp

ff_ref.o: ff_ref.cpp
	$(CXX) $(CXXFLAGS) -c ff_ref.cpp

ff_dicom3d.o: ff_dicom3d.cpp dicom.h dicom.o
	$(CXX) $(CXXFLAGS) -c ff_dicom3d.cpp

ff_dicom4d.o: ff_dicom4d.cpp dicom.h dicom.o
	$(CXX) $(CXXFLAGS) -c ff_dicom4d.cpp

ff_nifti3d.o: ff_nifti3d.cpp nifti.h nifti.o nifti11_ref.h
	$(CXX) $(CXXFLAGS) -c ff_nifti3d.cpp

ff_nifti4d.o: ff_nifti4d.cpp nifti.h nifti.o nifti11_ref.h
	$(CXX) $(CXXFLAGS) -c ff_nifti4d.cpp

ff_img3d.o: ff_img3d.cpp analyze.h analyze.o
	$(CXX) $(CXXFLAGS) -c ff_img3d.cpp

ff_img4d.o: ff_img4d.cpp analyze.h analyze.o
	$(CXX) $(CXXFLAGS) -c ff_img4d.cpp

ff_imgdir.o: ff_imgdir.cpp analyze.h analyze.o
	$(CXX) $(CXXFLAGS) -c ff_imgdir.cpp

ff_roi.o: ff_roi.cpp
	$(CXX) $(CXXFLAGS) -c ff_roi.cpp

ff_ge.o: ff_ge.cpp
	$(CXX) $(CXXFLAGS) -c ff_ge.cpp

ff_vmp3d.o: ff_vmp3d.cpp
	$(CXX) $(CXXFLAGS) -c ff_vmp3d.cpp

# rule that should be good for most of our .o files

%.o : %.cpp vbio.h vbutil.h vb_vector.h
	$(CXX) $(CXXFLAGS) -c $<

# test program

test:	libvbio.a test.cpp libvbprefs.a libvbglm.a
	$(CXX) $(CXXFLAGS) -o test test.cpp $(LIBS)
