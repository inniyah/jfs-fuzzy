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

PROGRAMS = \
	$(BIN_DIR)jfg_ex01 $(BIN_DIR)jfg_ex02 $(BIN_DIR)jfg_ex03 \
	$(BIN_DIR)jfg_ex04 $(BIN_DIR)jfp_ex01 $(BIN_DIR)jfp_ex02 \
	$(BIN_DIR)jfp_ex03 $(BIN_DIR)jfp_ex04 \
	$(BIN_DIR)jfr_ex01 $(BIN_DIR)jfr_ex02

all: bin $(PROGRAMS)

bin:
	mkdir -p bin

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
