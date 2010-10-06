#  (C) 2005 Miriam Ruiz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

all: tmp-stamp-jfs tmp-stamp-runa tmp-stamp-learna \
	tmp-stamp-jdoc tmp-stamp-source1

JHLP= bin/jhlp

$(JHLP):
	cd ../source; make ../doc/bin/jhlp

JFS_FILES= jfs.jhp jfs.jhd jfs.jhn intro.jhd started.jhd jfl.jhp jfldecl.jhd \
	jflprog.jhd jfsw.jhp jfsw.jhd jfc.jhp jfc.jhd jfr2s.jhp jfr2s.jhd \
	jfr.jhp jfr.jhd jhlp.jhp jhlp.jhd and.gif brake.gif height.gif overview.gif

# documentation for basic system
tmp-stamp-jfs: $(JHLP) jfs.jhc $(JFS_FILES)
	$(JHLP) -c -hi jfs -e tmp-install.log -a  -si html/jfs
	touch tmp-stamp-jfs

RUNA_FILES= runa.jhp jfrw.jhp jfrw.jhd jfr2c.jhp jfr2c.jhd jfr2htm.jhp \
	jfr2htm.jhd jfr2hdem.txt jfr2hdem.htm jfrl200.jhp jfrl200.jhd

# documentation for conversition tools
tmp-stamp-runa: $(JHLP) runa.jhc $(RUNA_FILES)
	$(JHLP) -c -hi runa -e tmp-install.log -a -si html/jfs
	touch tmp-stamp-runa

LEARNA_FILES= learna.jhp learna.jhd jfi.jhp jfi.jhd jfgp.jhp jfgp.jhd \
	jfea.jhp jfea.jhd jfrd.jhp jfrd.jhd jfid3.jhp jfid3.jhd \
	jffam.jhp jffam.jhd sina.gif sinb.gif

# documentation for learning tools
tmp-stamp-learna: $(JHLP) learna.jhc $(LEARNA_FILES)
	$(JHLP) -c -hi learna -e tmp-install.log -a -si html/jfs
	touch tmp-stamp-learna

JDOC_FILES= jdoc.jhp jfplot.jhp jfplot.jhd jfrplt.jhp jfrplt.jhd \
	brake3b.gif brake15b.gif

# documentation for documentation tools
tmp-stamp-jdoc: $(JHLP) jdoc.jhc $(JDOC_FILES)
	$(JHLP) -c -hi jdoc -e tmp-install.log -a -si html/jfs
	touch tmp-stamp-jdoc

SOURCE1_FILES= source1.jhp source1.jhd jfr_lib.jhd jfg_lib.jhd jfp_lib.jhd

# documentation for source code
tmp-stamp-source1: $(JHLP) source1.jhc $(SOURCE1_FILES)
	$(JHLP) -c -hi source1 -e tmp-install.log -a -si html/jfs
	touch tmp-stamp-source1

clean:
	rm -f html/*
	rm -rf bin
	rm -f tmp-*

distclean: clean
