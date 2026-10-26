CC ?= gcc
CXX ?= g++
PLUGIN_DIR := $(shell $(CC) -print-file-name=plugin)

CFLAGS ?= -O2 -Wall
CXXFLAGS ?= -O2 -Wall

all: gcc-record.so gcc-record-ld.so

gcc-record.so: src/gcc-record.cc
	$(CXX) $(CXXFLAGS) -shared -fPIC -fno-rtti -I$(PLUGIN_DIR)/include -o $@ $<

gcc-record-ld.so: src/gcc-record-ld.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

test: all
	cd tests && rm -f *.gcc-record hello.o hello && \
	$(CC) -fplugin=../gcc-record.so -c hello.c && \
	$(CC) -Wl,-plugin=../gcc-record-ld.so -o hello hello.o && \
	echo "== hello.c.gcc-record" && cat hello.c.gcc-record && \
	echo "== hello.gcc-record" && cat hello.gcc-record

clean:
	rm -f *.so tests/*.gcc-record tests/hello.o tests/hello

.PHONY: all test clean
