.PHONY: all clean tests

all: libsequenceparser.a lss

CC=g++
CFLAGS=-Iinclude -std=c++0x -Wall
CFLAGS+=-O3
#CFLAGS+=-DDEBUG -g -O0

lss: app/lss.cpp libsequenceparser.a
	$(CC) $(CFLAGS) $^ -o $@

tests: all_tests
	./$^ --gtest_filter=-Performance*

all_tests: test/tests.cpp libsequenceparser.a
	$(CC) $(CFLAGS) $^ -o $@ -lgtest_main

libsequenceparser.a: FolderParser.o
	ar -cvq libsequenceparser.a FolderParser.o

FolderParser.o: src/FolderParser.cpp
	$(CC) $(CFLAGS) $^ -c -o $@

clean:
	rm -f *.o *.a lss all_tests