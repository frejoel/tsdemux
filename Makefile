CC = gcc
ODIR = bin
CCDIR = coverage
INSTALL_DIR = /usr/local
CCOBJDIR = $(CCDIR)/obj
CFLAGS = -Ibin -Lbin
LIBS = -ltsdemux

.SECONDEXPANSION:
OBJ_TESTS := $(patsubst %.c, %.o, $(wildcard test/*.c))
OBJ_EXAMPLES := $(patsubst %.c, %.o, $(wildcard examples/*.c))

DEBUG ?= 0
COVERAGE ?= 0
PROFILING ?= 0

ifeq ($(COVERAGE), 1)
	CFLAGS += -fprofile-arcs -ftest-coverage -fprofile-dir=$(CCOBJDIR)
	DEBUG = 1
endif

ifeq ($(PROFILING), 1)
	CFLAGS += -pg
	DEBUG = 1
endif

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g
else
	CFLAGS += -O2
endif

all: static tests examples

.PHONY: style static tests check clean

style:
	astyle --style=linux -n src/*.h src/*.c

static:
	mkdir -p $(ODIR)
	$(CC) -c -o $(ODIR)/libtsdemux.a $(CFLAGS) src/tsdemux.c
	cp src/tsdemux.h $(ODIR)/tsdemux.h

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

install: static
	mkdir -p $(INSTALL_DIR)/include/libtsdemux
	mkdir -p $(INSTALL_DIR)/lib
	cp $(ODIR)/tsdemux.h $(INSTALL_DIR)/include/libtsdemux/
	cp $(ODIR)/libtsdemux.a $(INSTALL_DIR)/lib/

remove:
	rm -f $(INSTALL_DIR)/include/libtsdemux/libtsdemux.h
	rm -r -f $(INSTALL_DIR)/include/libtsdemux
	rm -f $(INSTALL_DIR)/lib/libtsdemux.a

tests: static $(OBJ_TESTS)

examples: static $(OBJ_EXAMPLES)

check: tests $(OBJ_TESTS)
	./test-runner.sh

ifeq ($(COVERAGE), 1)
	mkdir -p $(CCDIR)
	rm -f *.gcno
	rm -f $(CCOBJDIR)/*.gcda
	mv bin/*.gcno $(CCOBJDIR)/bin/
	lcov --directory $(CCOBJDIR) --capture --output-file $(CCDIR)/coverage.info
	genhtml --output-directory $(CCDIR)/html $(CCDIR)/coverage.info
endif

clean:
	rm -f -r $(ODIR) $(CCDIR)
	find . -type f -name '*.o' -exec rm {} \;
	find . -type f -name '*.o.dSYM' -exec rm {} \;
	find . -type f -name '*.o.gcno' -exec rm {} \;
	find . -type f -name '*.o.gcda' -exec rm {} \;
	find . -type f -name 'gmon.out' -exec rm {} \;
