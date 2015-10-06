# Makefile for Sportident various test programs

TESTS = test_framing test_device_find test_reader

PROGRAMS = reader

CC = gcc
CFLAGS = -pedantic -Wall -Wstrict-prototypes -std=gnu99 -g -D DEBUG
LIB = sportident.a

all: $(TESTS)

reader: reader.o $(LIB)
	$(CC) -o $@ $^

test_select: test_select.o $(LIB)
	$(CC) -o $@ $^

test_device_find: test_device_find.o $(LIB)
	$(CC) -o $@ $^

test_framing: test_framing.o $(LIB)
	$(CC) -o $@ $^

test_reader: test_reader.o $(LIB)
	$(CC) -o $@ $^

$(LIB): $(LIB)(sportident.o)

sportident.o: sportident.h si_const.h

reader.o: reader.c sportident.h

test_select.o: test_select.c sportident.h

test_framing.o: test_framing.c sportident.h

test_device_find.o: test_device_find.c sportident.h

test_reader.o: test_reader.c sportident.h

clean:
	-rm *.o $(LIB) $(TESTS) $(PROGRAMS)
