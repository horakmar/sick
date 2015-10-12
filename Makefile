# Makefile for Sportident various test programs

SI_LIBS = si_base.o si_decoder.o si_reader.o si_readloop.o si_print.o

PROGRAMS = test_reader connector_fifo_print

CC = gcc
CFLAGS = -pedantic -Wall -Wstrict-prototypes -std=gnu99 -g -D DEBUG
LIB = si_base.a

all: $(PROGRAMS)

test_reader: test_reader.o $(LIB)
	$(CC) -o $@ $^

connector_fifo_print: connector_fifo_print.o $(LIB)
	$(CC) -o $@ $^

$(LIB): $(LIB)($(SI_LIBS))

si_base.o: si_base.c si_base.h si_const.h

test_reader.o: test_reader.c si_base.h si_const.h

si_decoder.o: si_decoder.c si_base.h si_const.h

si_reader.o: si_reader.c si_base.h si_const.h

si_readloop.o: si_readloop.c si_base.h si_const.h

si_print.o: si_print.c si_base.h si_const.h si_print.h

connector_fifo_print.o: connector_fifo_print.c si_base.h si_const.h

clean:
	-rm *.o $(LIB) $(PROGRAMS)
