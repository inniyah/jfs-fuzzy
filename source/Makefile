#  (C) 2005-2010 Miriam Ruiz
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

LIBRARY=libjfs
LIBXTRA=libjfs-xtra

MAJOR=2
MINOR=0

LIB_DIR = ../lib
BIN_DIR = ../bin
BIN_PREFIX = $(BIN_DIR)/jfs-

all: libs programs

CC = gcc
CPP = g++
AR= ar rcu
RANLIB= ranlib
#RANLIB= ar s

# WIN32
RESFLAGS    = --include-dir /mingw/include --include-dir . --define __WIN32__ --define __WIN95__ --define __GNUWIN32__
RESCOMP     = windres

WARN= -pedantic -Wall -ansi
OPT= -O2 -g
INCS=
DEFS=
LIBS= -L$(LIB_DIR) -ljfs-xtra -ljfs -lm

EXTRA_LDFLAGS=
EXTRA_CFLAGS=

LDFLAGS= -Wl,-z,defs -Wl,--as-needed -Wl,--no-undefined $(EXTRA_LDFLAGS)

# jft_lib = JFS-Tokeniser. Functions to read jfs-values from an ascii-file or from text-strings
# jfm_lib = Memory-management system. System of circular list with variable-sized data bound to each node.
# jopt_lib =  Functions to decode an option-string
# jfr_lib = JFS to create a fuzzy system from data-sets using the Wang-Mendel method combined with an ad-hoc rule reduction method.
# jfg_lib = C-library to get information about a compiled jfs-program.
# jfp_lib = Functions to change a compiled jfs-program.
# jfs_text = JFS constant-texts. 

LIBRARY_SOURCES= jft_lib.cpp jfm_lib.cpp jopt_lib.cpp jfr_lib.cpp jfg_lib.cpp \
	jfp_lib.cpp jfs_text.cpp

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

LIBXTRA_SOURCES = jfs2wlib.cpp jfw2rlib.cpp jfr2wlib.cpp jfw2slib.cpp \
	jhlp_lib.cpp jfpltlib.cpp jfr2clib.cpp jfr2hlib.cpp jfea_lib.cpp \
	jfid3lib.cpp jfrd_lib.cpp jffamlib.cpp jfgp_lib.cpp jfi_lib.cpp


LIBRARY_SHARED_OBJS = $(LIBRARY_SOURCES:.cpp=.shared.o)
LIBRARY_STATIC_OBJS = $(LIBRARY_SOURCES:.cpp=.static.o)
LIBXTRA_SHARED_OBJS = $(LIBXTRA_SOURCES:.cpp=.shared.o)
LIBXTRA_STATIC_OBJS = $(LIBXTRA_SOURCES:.cpp=.static.o)

libs: $(LIB_DIR)/$(LIBRARY).a $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBXTRA).a $(LIB_DIR)/$(LIBXTRA).so

$(LIB_DIR)/$(LIBRARY).so.$(MAJOR).$(MINOR): $(LIBRARY_SHARED_OBJS)
	$(CPP) $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBRARY).so.$(MAJOR) \
		$(LIBRARY_SHARED_OBJS) -o $@ 

$(LIB_DIR)/$(LIBRARY).so: $(LIB_DIR)/$(LIBRARY).so.$(MAJOR).$(MINOR)
	rm -f $@.$(MAJOR)
	ln -s $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	rm -f $@
	ln -s $@.$(MAJOR) $@

$(LIB_DIR)/$(LIBRARY).a: $(LIBRARY_STATIC_OBJS)
	$(AR) $@ $+

$(LIB_DIR)/$(LIBXTRA).so.$(MAJOR).$(MINOR): $(LIBXTRA_SHARED_OBJS) $(LIB_DIR)/$(LIBRARY).so
	$(CPP) $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBXTRA).so.$(MAJOR) \
		$(LIBXTRA_SHARED_OBJS) -o $@ -L$(LIB_DIR) -ljfs

$(LIB_DIR)/$(LIBXTRA).so: $(LIB_DIR)/$(LIBXTRA).so.$(MAJOR).$(MINOR)
	rm -f $@.$(MAJOR)
	ln -s $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	rm -f $@
	ln -s $@.$(MAJOR) $@

$(LIB_DIR)/$(LIBXTRA).a: $(LIBXTRA_STATIC_OBJS)
	$(AR) $@ $+

PROGRAMS = $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBXTRA).so \
	$(BIN_PREFIX)jfc $(BIN_PREFIX)jfr $(BIN_PREFIX)jfr2s $(BIN_PREFIX)jhlp \
	$(BIN_PREFIX)jfplot $(BIN_PREFIX)jfrplt $(BIN_PREFIX)jfr2c $(BIN_PREFIX)jfr2htm \
	$(BIN_PREFIX)jfea $(BIN_PREFIX)jfid3 $(BIN_PREFIX)jfrd $(BIN_PREFIX)jffam $(BIN_PREFIX)jfgp $(BIN_PREFIX)jfi

programs: $(PROGRAMS)

JFC_OBJS = jfc.o jfs2wlib.o jfw2rlib.o jfs_text.o
$(BIN_PREFIX)jfc: jfc.o
	$(CPP) -o $@ $< $(LIBS)

JFR_OBJS = jfr.o jfr_lib.o jfg_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfr: jfr.o
	$(CPP) -o $@ $< $(LIBS)

JFR2S_OBJS = jfr2s.o jfr2wlib.o jfw2slib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2s: jfr2s.o
	$(CPP) -o $@ $< $(LIBS)

JHLP_OBJS = jhlp.o jhlp_lib.o jfm_lib.o
$(BIN_PREFIX)jhlp: jhlp.o
	$(CPP) -o $@ $< $(LIBS)

../doc/bin/jhlp: $(JHLP_OBJS)
	test -d ../doc/bin/ || mkdir ../doc/bin/
	$(CPP) -o $@ $(JHLP_OBJS) -lm

JFPLOT_OBJS = jfplot.o jfpltlib.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfplot: jfplot.o
	$(CPP) -o $@ $< $(LIBS)

JFRPLT_OBJS = jfrplt.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o jft_lib.o
$(BIN_PREFIX)jfrplt: jfrplt.o
	$(CPP) -o $@ $< $(LIBS)

JFR2C_OBJS = jfr2c.o jfr2clib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2c: jfr2c.o
	$(CPP) -o $@ $< $(LIBS)

JFR2HTM_OBJS = jfr2htm.o jfr2hlib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2htm: jfr2htm.o
	$(CPP) -o $@ $< $(LIBS)

JFEA_OBJS = jfea.o jfea_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfea: jfea.o
	$(CPP) -o $@ $< $(LIBS)

JFID3_OBJS = jfid3.o jfid3lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfid3: jfid3.o
	$(CPP) -o $@ $< $(LIBS)

JFRD_OBJS = jfrd.o jfrd_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfrd: jfrd.o
	$(CPP) -o $@ $< $(LIBS)

JFFAM_OBJS = jffam.o jffamlib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jffam: jffam.o
	$(CPP) -o $@ $< $(LIBS)

JFGP_OBJS = jfgp.o jfgp_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfgp: jfgp.o
	$(CPP) -o $@ $< $(LIBS)

JFI_OBJS = jfi.o jfi_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfi: jfi.o
	$(CPP) -o $@ $< $(LIBS)

.SUFFIXES:	.o .c .cpp .rc _resources.o

CFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS) $(EXTRA_CFLAGS)
CPPFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS) $(EXTRA_CFLAGS)

.c.o :
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.o :
	$(CPP) -c $(CPPFLAGS) -o $@ $<

.rc_resources.o:
	$(RESCOMP) -i $< -o $@ $(RESFLAGS)


STATIC_CFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS) $(EXTRA_CFLAGS)
SHARED_CFLAGS= $(STATIC_CFLAGS) -fPIC

%.shared.o: %.cpp
	g++ -o $@ -c $+ $(SHARED_CFLAGS)

%.shared.o: %.c
	gcc -o $@ -c $+ $(SHARED_CFLAGS)

%.so : %.o
	g++ $(LDFLAGS) $(EXTRA_LDFLAGS) -shared $^ -o $@

%.static.o: %.cpp
	g++ -o $@ -c $+ $(STATIC_CFLAGS)

%.static.o: %.c
	gcc -o $@ -c $+ $(STATIC_CFLAGS)

clean: 
	rm -f *.o *~

distclean: clean
	rm -f $(PROGRAMS) $(LIB_DIR)/*.a $(LIB_DIR)/*.so $(LIB_DIR)/*.so.*
