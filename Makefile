CC=gcc
ODIR=bin
CFLAGS=-Ibin -Lbin -ltsdemux_d -g

TEST_FILES=test/parse_packet_header.o test/test_test.o

tsdemux:
	astyle --style=linux -n src/*.h src/*.c
	mkdir -p $(ODIR)
	$(CC) -c -o $(ODIR)/lib$@.a src/tsdemux.c
	$(CC) -c -o $(ODIR)/lib$@_d.a src/tsdemux.c -g
	cp src/tsdemux.h $(ODIR)/tsdemux.h

%.o: %.c tsdemux
	astyle --style=linux -n src/*.h src/*.c
	$(CC) -o $@ $< $(CFLAGS)

check: tsdemux $(TEST_FILES)
	./test/parse_packet_header.o

.PHONY: clean

clean:
	rm -f -r bin test/*.o test/*.o.dSYM
