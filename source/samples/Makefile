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

WARN= -pedantic -Wall -ansi
OPT= -O2 -g
INCS= -I.. -I/usr/include/jfs/
DEFS=
LIBS= -L.. -ljfs -lm

CFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS)
CPPFLAGS= $(OPT) $(WARN) $(INCS) $(DEFS)

# implementation

.SUFFIXES:	.o .c .cpp .rc _resources.o

.c.o :
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.o :
	$(CPP) -c $(CPPFLAGS) -o $@ $<

BIN_DIR = bin/

PROGRAMS = $(BIN_DIR)jfg_ex01 $(BIN_DIR)jfg_ex02 $(BIN_DIR)jfg_ex03 \
	$(BIN_DIR)jfg_ex04 $(BIN_DIR)jfp_ex01 $(BIN_DIR)jfp_ex02 \
	$(BIN_DIR)jfp_ex03 $(BIN_DIR)jfp_ex04 \
	$(BIN_DIR)jfr_ex01 $(BIN_DIR)jfr_ex02

all: bin $(PROGRAMS)

bin:
	test -d bin/ || mkdir bin

$(BIN_DIR)jfg_ex01: jfg_ex01.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfg_ex02: jfg_ex02.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfg_ex03: jfg_ex03.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfg_ex04: jfg_ex04.o
	$(CPP) -o $@ $< $(LIBS)

$(BIN_DIR)jfp_ex01: jfp_ex01.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfp_ex02: jfp_ex02.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfp_ex03: jfp_ex03.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfp_ex04: jfp_ex04.o
	$(CPP) -o $@ $< $(LIBS)

$(BIN_DIR)jfr_ex01: jfr_ex01.o
	$(CPP) -o $@ $< $(LIBS)
$(BIN_DIR)jfr_ex02: jfr_ex02.o
	$(CPP) -o $@ $< $(LIBS)

clean: 
	rm -f *.o *~

distclean: clean
	rm -f $(PROGRAMS)
	rm -rf bin
