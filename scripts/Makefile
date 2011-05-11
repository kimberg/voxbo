
# Makefile for shell scripts and other simple things

-include ../make_vars.txt
include ../make_stuff.txt

SHSCRIPTS=newprep.sh oldprep.sh minprep.sh makevlsm.sh vbinstall.sh
SHSCRIPTS+=spm8_coreg.sh spm8_realign.sh spm8_segment.sh spm8_warp.sh spm8_reslice.sh
SHSCRIPTS+=dcmcrawl.sh
BINSCRIPTS := $(basename $(SHSCRIPTS))

.PHONY: default install

default: $(BINSCRIPTS)

# turn the .sh files into unsuffixed executables

$(BINSCRIPTS): % : %.sh
	cp $< $@

install:
	cp -f $(BINSCRIPTS) $(VB_BINDIR)

clean:
	rm -f $(CLEANME) $(BINSCRIPTS)
