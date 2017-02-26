CC = gcc
ODIR = bin
CCDIR = coverage
CCOBJDIR = $(CCDIR)/obj
CFLAGS = -Ibin -Lbin
LIBS = -ltsdemux

TEST_FILES := test/parse_packet_header.o test/data_context.o test/parse_table.o test/parse_pat.o test/parse_pmt.o test/parse_cat.o test/register_pid.o test/parse_pes_header.o
EXAMPLE_FILES := examples/demux.o

DEBUG ?= 0
COVERAGE ?= 0

ifeq ($(COVERAGE), 1)
	CFLAGS += -fprofile-arcs -ftest-coverage -fprofile-dir=$(CCOBJDIR)
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
	astyle --style=linux -n src/*.h src/*.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

tests: static $(TEST_FILES)

examples: static $(EXAMPLE_FILES)

check: tests
	./test/parse_packet_header.o
	./test/data_context.o
	./test/parse_table.o
	./test/parse_pat.o
	./test/parse_pmt.o
	./test/parse_cat.o
	./test/register_pid.o
	./test/parse_pes_header.o
ifeq ($(COVERAGE), 1)
	mkdir -p $(CCDIR)
	rm -f *.gcno
	rm -f $(CCOBJDIR)/*.gcda
	mv bin/*.gcno $(CCOBJDIR)/bin/
	lcov --directory $(CCOBJDIR) --capture --output-file $(CCDIR)/coverage.info
	genhtml --output-directory $(CCDIR)/html $(CCDIR)/coverage.info
endif

clean:
	rm -f -r $(ODIR) $(CCDIR) **/*.o **/*.o.dSYM **/*.gcno **/*.gcda
