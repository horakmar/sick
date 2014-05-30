# Makefile for Sportident various test programs

TESTS = test_crc test_framing test_sizeof test_device_find test_handshake test_reader

CC = gcc
CFLAGS = -pedantic -Wall -Wstrict-prototypes -std=gnu99 -g -D DEBUG
LIB = sportident.a

all: $(TESTS)

test_select: test_select.o $(LIB)
	$(CC) -o $@ $^

test_device_find: test_device_find.o $(LIB)
	$(CC) -o $@ $^

test_framing: test_framing.o $(LIB)
	$(CC) -o $@ $^

test_handshake: test_handshake.o $(LIB)
	$(CC) -o $@ $^

siread: siread.o $(LIB)
	$(CC) -o $@ $^

test_reader: test_reader.o $(LIB)
	$(CC) -o $@ $^

test_crc: test_crc.o $(LIB)
	$(CC) -o $@ $^

$(LIB): $(LIB)(sportident.o)

test_select.o: test_select.c sportident.h

test_crc.o: test_crc.c sportident.h

test_framing.o: test_framing.c sportident.h

test_handshake.o: test_framing.c sportident.h

test_device_find.o: test_device_find.c sportident.h

siread.o: siread.c sportident.h

test_reader.o: test_reader.c sportident.h

clean:
	-rm *.o $(LIB) $(TESTS)
