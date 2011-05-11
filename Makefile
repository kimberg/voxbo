
# This is the master makefile for VoxBo and the database project

-include make_vars.txt
include make_stuff.txt

# Here are our targets

COMMANDLINEDIRS=fileformats munge stats crunch stand_alone utils resample gdscript\
                scheduler scripts

ifeq ($(VB_TARGET),spm)
  GUIDIRS= qtglm vbview
  DBDIRS=
else ifeq ($(VB_TARGET),db)
  COMMANDLINEDIRS=
  GUIDIRS=
  DBDIRS=client server dbutil
else
  GUIDIRS=qtglm vbview vbsequence qtvlsm
  DBDIRS=client server dbutil
endif

VOXBODIRS = $(COMMANDLINEDIRS) $(GUIDIRS)
INSTALLDIRS = $(COMMANDLINEDIRS) $(GUIDIRS)
MAKEDIRS = $(VOXBODIRS) $(DBDIRS)
CLEANDIRS = lib vbwidgets $(MAKEDIRS)
OUT=getput qa brainBrowser db_import

# PHONY declaration makes life easier
.PHONY: clean install lib vbwidgets subdirs db nogui voxbo showconfig $(VOXBODIRS) $(DBDIRS)

subdirs: lib $(VOXBODIRS) $(DBDIRS)

nogui: $(COMMANDLINEDIRS)

db: $(DBDIRS)

voxbo: $(VOXBODIRS)

# all targets
$(MAKEDIRS): lib
	+make -C $@

# extra dependencies for specific targets
qtglm: lib vbwidgets utils
vbview: lib vbwidgets
client: lib vbwidgets vbview
vbsequence: lib vbwidgets

lib:
	+make -C $@

vbwidgets:
	+make -C $@

install:
	mkdir -p $(VB_BINDIR)
	+for dir in $(INSTALLDIRS) ; do make -C $$dir install ; done

distclean: clean
	rm -f make_vars.txt

showconfig:
	@echo "VB_PREFIX="$(VB_PREFIX)
	@echo "VB_BINDIR="$(VB_BINDIR)
	@echo "VB_LIBDIR="$(VB_LIBDIR)
	@echo "VB_FFDIR="$(VB_FFDIR)
	@echo "VB_TARGET="$(VB_TARGET)

clean:
	+for dir in $(CLEANDIRS) ; do make -C $$dir clean ; done

