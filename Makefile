CC ?= gcc
CXX ?= g++
PLUGIN_DIR := $(shell $(CC) -print-file-name=plugin)

CFLAGS ?= -O2 -Wall
CXXFLAGS ?= -O2 -Wall

all: gcc_record.so gcc-record-ld.so

LIBS := -lcrypto -lz -lzstd

gcc_record.so: src/gcc-record.cc src/sha1file.h src/record-compress.h
	$(CXX) $(CXXFLAGS) -shared -fPIC -fno-rtti -I$(PLUGIN_DIR)/include -o $@ $< $(LIBS)

gcc-record-ld.so: src/gcc-record-ld.c src/sha1file.h src/record-compress.h
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< $(LIBS)

test: all
	sh tests/run.sh

clean:
	rm -f *.so tests/*.o tests/hello tests/unzstd

.PHONY: all test clean
