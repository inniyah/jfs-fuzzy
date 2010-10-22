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
LIBHLPR=libjfs-helpers

MAJOR=2
MINOR=0

LIB_DIR = ../lib
BIN_DIR = ../bin
PRG_PREFIX = jfs-
BIN_PREFIX = $(BIN_DIR)/$(PRG_PREFIX)

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
DEFS= -DPRG_PREFIX="$(PRG_PREFIX)"
LIBS= -L$(LIB_DIR) -ljfs-helpers -ljfs-core -lm

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
# cmds_lib = Common subroutines for the command-line applications

LIBHLPR_SOURCES = \
	helpers/jfs2wlib.c \
	helpers/jfw2rlib.c \
	helpers/jfr2wlib.c \
	helpers/jfw2slib.c \
	helpers/jhlp_lib.c \
	helpers/jfpltlib.c \
	helpers/jfr2clib.c \
	helpers/jfr2hlib.c \
	helpers/jfea_lib.c \
	helpers/jfid3lib.c \
	helpers/jfrd_lib.c \
	helpers/jffamlib.c \
	helpers/jfgp_lib.c \
	helpers/jfi_lib.c \
	helpers/cmds_lib.c

LIBRARY_SHARED_OBJS = $(LIBRARY_SOURCES:.c=.shared.o)
LIBRARY_STATIC_OBJS = $(LIBRARY_SOURCES:.c=.static.o)
LIBHLPR_SHARED_OBJS = $(LIBHLPR_SOURCES:.c=.shared.o)
LIBHLPR_STATIC_OBJS = $(LIBHLPR_SOURCES:.c=.static.o)

libs: $(LIB_DIR)/$(LIBRARY).a $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBHLPR).a $(LIB_DIR)/$(LIBHLPR).so

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

$(LIB_DIR)/$(LIBHLPR).so.$(MAJOR).$(MINOR): $(LIBHLPR_SHARED_OBJS) $(LIB_DIR)/$(LIBRARY).so
	mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBHLPR).so.$(MAJOR) \
		$(LIBHLPR_SHARED_OBJS) -o $@ -L$(LIB_DIR) -ljfs-core -lm

$(LIB_DIR)/$(LIBHLPR).so: $(LIB_DIR)/$(LIBHLPR).so.$(MAJOR).$(MINOR)
	mkdir -p $(LIB_DIR)
	cd $(LIB_DIR) && rm -fv $(LIBHLPR).so.$(MAJOR)
	cd $(LIB_DIR) && ln -s $(LIBHLPR).so.$(MAJOR).$(MINOR) $(LIBHLPR).so.$(MAJOR)
	cd $(LIB_DIR) && rm -fv $(LIBHLPR).so
	cd $(LIB_DIR) && ln -s $(LIBHLPR).so.$(MAJOR) $(LIBHLPR).so

$(LIB_DIR)/$(LIBHLPR).a: $(LIBHLPR_STATIC_OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) $@ $+

PROGRAMS = $(LIB_DIR)/$(LIBRARY).so $(LIB_DIR)/$(LIBHLPR).so \
	$(BIN_PREFIX)jfc $(BIN_PREFIX)jfr $(BIN_PREFIX)jfr2s $(BIN_PREFIX)jhlp \
	$(BIN_PREFIX)jfplot $(BIN_PREFIX)jfrplt $(BIN_PREFIX)jfr2c $(BIN_PREFIX)jfr2htm \
	$(BIN_PREFIX)jfea $(BIN_PREFIX)jfid3 $(BIN_PREFIX)jfrd $(BIN_PREFIX)jffam $(BIN_PREFIX)jfgp $(BIN_PREFIX)jfi

programs: $(PROGRAMS)

#JFC_OBJS = jfc.o jfs2wlib.o jfw2rlib.o jfs_text.o
$(BIN_PREFIX)jfc: commands/jfc.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFR_OBJS = jfr.o jfr_lib.o jfg_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfr: commands/jfr.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFR2S_OBJS = jfr2s.o jfr2wlib.o jfw2slib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2s: commands/jfr2s.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JHLP_OBJS = jhlp.o jhlp_lib.o jfm_lib.o
$(BIN_PREFIX)jhlp: commands/jhlp.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

JHLP_OBJS = commands/jhlp.o helpers/jhlp_lib.o helpers/cmds_lib.o core/jfm_lib.o
../manual/bin/jhlp: $(JHLP_OBJS)
	mkdir -p ../manual/bin/
	$(CC) -o $@ $(JHLP_OBJS) -lm

#JFPLOT_OBJS = jfplot.o jfpltlib.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfplot: commands/jfplot.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFRPLT_OBJS = jfrplt.o jopt_lib.o jfr_lib.o jfg_lib.o jfs_text.o jft_lib.o
$(BIN_PREFIX)jfrplt: commands/jfrplt.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFR2C_OBJS = jfr2c.o jfr2clib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2c: commands/jfr2c.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFR2HTM_OBJS = jfr2htm.o jfr2hlib.o jfr_lib.o jfg_lib.o jfs_text.o
$(BIN_PREFIX)jfr2htm: commands/jfr2htm.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFEA_OBJS = jfea.o jfea_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfea: commands/jfea.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFID3_OBJS = jfid3.o jfid3lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfid3: commands/jfid3.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFRD_OBJS = jfrd.o jfrd_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfrd: commands/jfrd.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFFAM_OBJS = jffam.o jffamlib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jffam: commands/jffam.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFGP_OBJS = jfgp.o jfgp_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfgp: commands/jfgp.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

#JFI_OBJS = jfi.o jfi_lib.o jfg_lib.o jfp_lib.o jfr_lib.o jft_lib.o jfs_text.o
$(BIN_PREFIX)jfi: commands/jfi.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(LIBS)

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
	rm -f *.o core/*.o helpers/*.o commands/*.o *~

distclean: clean
	rm -f $(PROGRAMS) $(LIB_DIR)/*.a $(LIB_DIR)/*.so $(LIB_DIR)/*.so.*
