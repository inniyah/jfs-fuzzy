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

CC = gcc
CPP = g++
AR= ar rcu
RANLIB= ranlib
#RANLIB= ar s

WARN= -pedantic -Wall -ansi
OPT= -O2 -g
INCS=
DEFS=
LIBS= -L. -ljfs-xtra -ljfs -lm

CFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS)
CPPFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS)

# WIN32
RESFLAGS    = --include-dir /mingw/include --include-dir . --define __WIN32__ --define __WIN95__ --define __GNUWIN32__
RESCOMP     = windres

# implementation

.SUFFIXES:	.o .c .cpp .rc _resources.o

.c.o :
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.o :
	$(CPP) -c $(CPPFLAGS) -o $@ $<

.rc_resources.o:
	$(RESCOMP) -i $< -o $@ $(RESFLAGS)

BIN_DIR = ../bin/jfs-

PROGRAMS = libjfs.so libjfs-xtra.a \
	$(BIN_DIR)jfc $(BIN_DIR)jfr $(BIN_DIR)jfr2s $(BIN_DIR)jhlp \
	$(BIN_DIR)jfplot $(BIN_DIR)jfrplt $(BIN_DIR)jfr2c $(BIN_DIR)jfr2htm \
	$(BIN_DIR)jfea $(BIN_DIR)jfid3 $(BIN_DIR)jfrd $(BIN_DIR)jffam $(BIN_DIR)jfgp $(BIN_DIR)jfi

all: $(PROGRAMS)

# jft_lib = JFS-Tokeniser. Functions to read jfs-values from an ascii-file or from text-strings
# jfm_lib = Memory-management system. System of circular list with variable-sized data bound to each node.
# jopt_lib =  Functions to decode an option-string
# jfr_lib = JFS to create a fuzzy system from data-sets using the Wang-Mendel method combined with an ad-hoc rule reduction method.
# jfg_lib = C-library to get information about a compiled jfs-program.
# jfp_lib = Functions to change a compiled jfs-program.
# jfs_text = JFS constant-texts. 
LIB_OBJS = jft_lib.o jfm_lib.o jopt_lib.o jfr_lib.o jfg_lib.o jfp_lib.o \
	jfs_text.o

libjfs.a: $(LIB_OBJS)
	$(AR) $@ $?
	$(RANLIB) $@

libjfs.so: $(LIB_OBJS)
	$(CPP) -shared -o $@.2 $? -Wl,-soname,libjfs.so.2
	ln -s $@.2 $@

jft_lib.o: jft_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jfm_lib.o: jfm_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jopt_lib.o: jopt_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jfr_lib.o: jfr_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jfg_lib.o: jfg_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jfp_lib.o: jfp_lib.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic
jfs_text.o: jfs_text.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $< -fpic

# jfs2wlib = JFS converter. Converts a JFS-file to a JFW-file.
# jfw2rlib = JFS converter. Converts a JFW-file to a JFR-file.
# jfr2wlib = JFS Inverse Compiler-functions. Converts a JFR-file to a JFW-file.
# jfw2slib = JFS Inverse Compiler-functions. Converts a JFW-file to a JFS-file.
# jhlp_lib = Converts a jhlp-system to html.
# jfpltlib = Functions to write plot-info about a compiled jfs-program to GNU-plot.
# jfr2clib = Functions to convert a compiled jfs-program to C-sourcecode.
# jfr2hlib = JFS JFR-to-html (javascript) converter.
# jfea_lib = JFS  rule creator using evolutionary programing.
# jfid3lib = JFS Ruled discover using ID3.
# jfrd_lib = JFS to create a fuzzy system from data-sets using the Wang-Mendel method combined with an ad-hoc rule reduction method.
# jffamlib = JFS Fam-creation by a cellular automat.
# jfgp_lib = JFS rule discover-functions using Genetic programing.
# jfi_lib = JFS  improver-functions using evolutionary programing.
LIBX_OBJS = jfs2wlib.o jfw2rlib.o jfr2wlib.o jfw2slib.o \
	jhlp_lib.o jfpltlib.o jfr2clib.o jfr2hlib.o jfea_lib.o \
	jfid3lib.o jfrd_lib.o jffamlib.o jfgp_lib.o jfi_lib.o

libjfs-xtra.a: $(LIBX_OBJS)
	$(AR) $@ $?
	$(RANLIB) $@

JFC_OBJS = jfc.o jfs2wlib.o jfw2rlib.o jfs_text.o
$(BIN_DIR)jfc: jfc.o
	$(CPP) -o $@ $< $(LIBS)

JFR_OBJS = jfr.o jfr_lib.o jfg_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfr: jfr.o
	$(CPP) -o $@ $< $(LIBS)

JFR2S_OBJS = jfr2s.o jfr2wlib.o jfw2slib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_DIR)jfr2s: jfr2s.o
	$(CPP) -o $@ $< $(LIBS)

JHLP_OBJS = jhlp.o jhlp_lib.o jfm_lib.o
$(BIN_DIR)jhlp: jhlp.o
	$(CPP) -o $@ $< $(LIBS)

../doc/bin/jhlp: $(JHLP_OBJS)
	test -d ../doc/bin/ || mkdir ../doc/bin/
	$(CPP) -o $@ $(JHLP_OBJS) -lm

JFPLOT_OBJS = jfplot.o jfpltlib.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_DIR)jfplot: jfplot.o
	$(CPP) -o $@ $< $(LIBS)

JFRPLT_OBJS = jfrplt.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o jft_lib.o
$(BIN_DIR)jfrplt: jfrplt.o
	$(CPP) -o $@ $< $(LIBS)

JFR2C_OBJS = jfr2c.o jfr2clib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_DIR)jfr2c: jfr2c.o
	$(CPP) -o $@ $< $(LIBS)

JFR2HTM_OBJS = jfr2htm.o jfr2hlib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_DIR)jfr2htm: jfr2htm.o
	$(CPP) -o $@ $< $(LIBS)

JFEA_OBJS = jfea.o jfea_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfea: jfea.o
	$(CPP) -o $@ $< $(LIBS)

JFID3_OBJS = jfid3.o jfid3lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfid3: jfid3.o
	$(CPP) -o $@ $< $(LIBS)

JFRD_OBJS = jfrd.o jfrd_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfrd: jfrd.o
	$(CPP) -o $@ $< $(LIBS)

JFFAM_OBJS = jffam.o jffamlib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jffam: jffam.o
	$(CPP) -o $@ $< $(LIBS)

JFGP_OBJS = jfgp.o jfgp_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfgp: jfgp.o
	$(CPP) -o $@ $< $(LIBS)

JFI_OBJS = jfi.o jfi_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_DIR)jfi: jfi.o
	$(CPP) -o $@ $< $(LIBS)

clean: 
	rm -f *.o *~

distclean: clean
	rm -f $(PROGRAMS) *.a *.so *.so.*
