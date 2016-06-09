# Makefile for Sportident various test programs

SI_LIBS = si_base.o si_decoder.o si_reader.o si_readloop.o si_print.o si_json.o

PROGRAMS = test_reader connector_fifo_print fifo_reader connector_fifo_onlyprint connector_socket_print socket_reader reader_curl

CC = gcc
CFLAGS = -pedantic -Wall -Wstrict-prototypes -std=gnu99 -g -D DEBUG
ARFLAGS = rvU
LIB = si_base.a

all: $(PROGRAMS)

reader_curl: reader_curl.o $(LIB)
	$(CC) -o $@ $^ -l json-c -l curl

test_reader: test_reader.o $(LIB)
	$(CC) -o $@ $^

connector_fifo_print: connector_fifo_print.o $(LIB)
	$(CC) -o $@ $^

fifo_reader: fifo_reader.o $(LIB)
	$(CC) -o $@ $^

connector_fifo_onlyprint: connector_fifo_onlyprint.o $(LIB)
	$(CC) -o $@ $^

connector_socket_print: connector_socket_print.o $(LIB)
	$(CC) -o $@ $^

socket_reader: socket_reader.o $(LIB)
	$(CC) -o $@ $^

$(LIB): $(LIB)($(SI_LIBS))

si_base.o: si_base.c si_base.h si_const.h

reader_curl.o: reader_curl.c si_base.h si_const.h si_print.h

test_reader.o: test_reader.c si_base.h si_const.h

si_decoder.o: si_decoder.c si_base.h si_const.h

si_reader.o: si_reader.c si_base.h si_const.h

si_readloop.o: si_readloop.c si_base.h si_const.h

si_print.o: si_print.c si_base.h si_const.h si_print.h

si_json.o: si_json.c si_base.h

connector_fifo_print.o: connector_fifo_print.c si_base.h si_const.h si_print.h

connector_fifo_onlyprint.o: connector_fifo_onlyprint.c si_base.h si_const.h si_print.h

fifo_reader.o: fifo_reader.c si_base.h si_const.h

connector_socket_print.o: connector_socket_print.c si_base.h si_const.h si_print.h

socket_reader.o: fifo_reader.c si_base.h si_const.h

clean:
	-rm *.o $(LIB) $(PROGRAMS)
