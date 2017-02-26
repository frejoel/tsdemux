CC=gcc
ODIR=bin
CFLAGS=-O0 -Ibin -Lbin -ltsdemux_d -g

TEST_FILES:=test/parse_packet_header.o test/data_context.o test/parse_table.o test/parse_pat.o test/parse_pmt.o test/parse_cat.o test/register_pid.o test/parse_pes_header.o
EXAMPLE_FILES:=examples/demux.o

all: tsdemux tests examples

.PHONY: style tsdemux tests check clean

style:
	astyle --style=linux -n src/*.h src/*.c

tsdemux:
	mkdir -p $(ODIR)
	$(CC) -c -o $(ODIR)/lib$@.a src/tsdemux.c
	$(CC) -c -o $(ODIR)/lib$@_d.a src/tsdemux.c -g
	cp src/tsdemux.h $(ODIR)/tsdemux.h

%.o: %.c tsdemux
	astyle --style=linux -n src/*.h src/*.c
	$(CC) -o $@ $< $(CFLAGS)

tests: tsdemux $(TEST_FILES)

examples: tsdemux $(EXAMPLE_FILES)

check: tests
	./test/parse_packet_header.o
	./test/data_context.o
	./test/parse_table.o
	./test/parse_pat.o
	./test/parse_pmt.o
	./test/parse_cat.o
	./test/register_pid.o
	./test/parse_pes_header.o

clean:
	rm -f -r bin test/*.o test/*.o.dSYM examples/*.o
