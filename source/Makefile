#  Copyright (C) 2005-2010 Miriam Ruiz
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


LIBRARY=libjfs
LIBXTRA=libjfs-helpers

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

WARN?= -pedantic -Wall -ansi
OPT?= -O2 -g
INCS= -Iinclude
DEFS=
LIBS= -L$(LIB_DIR) -ljfs-xtra -ljfs-core -lm

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

LIBRARY_SOURCES= \
	core/jft_lib.c \
	core/jfm_lib.c \
	core/jopt_lib.c \
	core/jfr_lib.c \
	core/jfg_lib.c \
	core/jfp_lib.c \
	core/jfs_text.c

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

LIBXTRA_SOURCES = \
	xtra/jfs2wlib.c \
	xtra/jfw2rlib.c \
	xtra/jfr2wlib.c \
	xtra/jfw2slib.c \
	xtra/jhlp_lib.c \
	xtra/jfpltlib.c \
	xtra/jfr2clib.c \
	xtra/jfr2hlib.c \
	xtra/jfea_lib.c \
	xtra/jfid3lib.c \
	xtra/jfrd_lib.c \
	xtra/jffamlib.c \
	xtra/jfgp_lib.c \
	xtra/jfi_lib.c

LIBRARY_SHARED_OBJS = $(LIBRARY_SOURCES:.c=.shared.o)
LIBRARY_STATIC_OBJS = $(LIBRARY_SOURCES:.c=.static.o)
LIBXTRA_SHARED_OBJS = $(LIBXTRA_SOURCES:.c=.shared.o)
LIBXTRA_STATIC_OBJS = $(LIBXTRA_SOURCES:.c=.static.o)

libs: $(LIB_DIR)/$(LIBRARY).a $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBXTRA).a $(LIB_DIR)/$(LIBXTRA).so

$(LIB_DIR)/$(LIBRARY).so.$(MAJOR).$(MINOR): $(LIBRARY_SHARED_OBJS)
	mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBRARY).so.$(MAJOR) \
		$(LIBRARY_SHARED_OBJS) -o $@ -lm

$(LIB_DIR)/$(LIBRARY).so: $(LIB_DIR)/$(LIBRARY).so.$(MAJOR).$(MINOR)
	mkdir -p $(LIB_DIR)
	cd $(LIB_DIR) && rm -fv $(LIBRARY).so.$(MAJOR)
	cd $(LIB_DIR) && ln -s $(LIBRARY).so.$(MAJOR).$(MINOR) $(LIBRARY).so.$(MAJOR)
	cd $(LIB_DIR) && rm -fv $(LIBRARY).so
	cd $(LIB_DIR) && ln -s $(LIBRARY).so.$(MAJOR) $(LIBRARY).so

$(LIB_DIR)/$(LIBRARY).a: $(LIBRARY_STATIC_OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) $@ $+

$(LIB_DIR)/$(LIBXTRA).so.$(MAJOR).$(MINOR): $(LIBXTRA_SHARED_OBJS) $(LIB_DIR)/$(LIBRARY).so
	mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBXTRA).so.$(MAJOR) \
		$(LIBXTRA_SHARED_OBJS) -o $@ -L$(LIB_DIR) -ljfs-core -lm

$(LIB_DIR)/$(LIBXTRA).so: $(LIB_DIR)/$(LIBXTRA).so.$(MAJOR).$(MINOR)
	mkdir -p $(LIB_DIR)
	cd $(LIB_DIR) && rm -fv $(LIBXTRA).so.$(MAJOR)
	cd $(LIB_DIR) && ln -s $(LIBXTRA).so.$(MAJOR).$(MINOR) $(LIBXTRA).so.$(MAJOR)
	cd $(LIB_DIR) && rm -fv $(LIBXTRA).so
	cd $(LIB_DIR) && ln -s $(LIBXTRA).so.$(MAJOR) $(LIBXTRA).so

$(LIB_DIR)/$(LIBXTRA).a: $(LIBXTRA_STATIC_OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) $@ $+

PROGRAMS = $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBXTRA).so \
	$(BIN_PREFIX)jfc $(BIN_PREFIX)jfr $(BIN_PREFIX)jfr2s $(BIN_PREFIX)jhlp \
	$(BIN_PREFIX)jfplot $(BIN_PREFIX)jfrplt $(BIN_PREFIX)jfr2c $(BIN_PREFIX)jfr2htm \
	$(BIN_PREFIX)jfea $(BIN_PREFIX)jfid3 $(BIN_PREFIX)jfrd $(BIN_PREFIX)jffam $(BIN_PREFIX)jfgp $(BIN_PREFIX)jfi

programs: $(PROGRAMS)

#JFC_OBJS = jfc.o jfs2wlib.o jfw2rlib.o jfs_text.o
$(BIN_PREFIX)jfc: cmds/jfc.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFR_OBJS = jfr.o jfr_lib.o jfg_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfr: cmds/jfr.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFR2S_OBJS = jfr2s.o jfr2wlib.o jfw2slib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2s: cmds/jfr2s.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JHLP_OBJS = jhlp.o jhlp_lib.o jfm_lib.o
$(BIN_PREFIX)jhlp: cmds/jhlp.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

JHLP_OBJS = cmds/jhlp.o xtra/jhlp_lib.o core/jfm_lib.o cmds/cmds_common.o
../doc/bin/jhlp: $(JHLP_OBJS)
	mkdir -p ../doc/bin/
	$(CC) -o $@ $(JHLP_OBJS) -lm

#JFPLOT_OBJS = jfplot.o jfpltlib.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfplot: cmds/jfplot.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFRPLT_OBJS = jfrplt.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o jft_lib.o
$(BIN_PREFIX)jfrplt: cmds/jfrplt.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFR2C_OBJS = jfr2c.o jfr2clib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2c: cmds/jfr2c.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFR2HTM_OBJS = jfr2htm.o jfr2hlib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2htm: cmds/jfr2htm.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFEA_OBJS = jfea.o jfea_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfea: cmds/jfea.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFID3_OBJS = jfid3.o jfid3lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfid3: cmds/jfid3.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFRD_OBJS = jfrd.o jfrd_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfrd: cmds/jfrd.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFFAM_OBJS = jffam.o jffamlib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jffam: cmds/jffam.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFGP_OBJS = jfgp.o jfgp_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfgp: cmds/jfgp.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

#JFI_OBJS = jfi.o jfi_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfi: cmds/jfi.o cmds/cmds_common.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $+ $(LIBS)

.SUFFIXES: .o .c .cpp .rc _resources.o

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
	$(CPP) -o $@ -c $+ $(SHARED_CFLAGS)

%.shared.o: %.c
	$(CC) -o $@ -c $+ $(SHARED_CFLAGS)

%.so : %.o
	$(CC) $(LDFLAGS) $(EXTRA_LDFLAGS) -shared $^ -o $@

%.static.o: %.cpp
	$(CPP) -o $@ -c $+ $(STATIC_CFLAGS)

%.static.o: %.c
	$(CC) -o $@ -c $+ $(STATIC_CFLAGS)

clean: 
	rm -f *.o core/*.o xtra/*.o cmds/*.o *~

distclean: clean
	rm -f $(PROGRAMS) $(LIB_DIR)/*.a $(LIB_DIR)/*.so $(LIB_DIR)/*.so.*
