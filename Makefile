CC ?= gcc
CXX ?= g++
PLUGIN_DIR := $(shell $(CC) -print-file-name=plugin)

CFLAGS ?= -O2 -Wall
CXXFLAGS ?= -O2 -Wall

all: gcc_record.so gcc-record-ld.so

LIBS := -lcrypto -lz -lzstd

gcc_record.so: src/gcc-record.cc src/sha1file.h src/record-write.h
	$(CXX) $(CXXFLAGS) -shared -fPIC -fno-rtti -I$(PLUGIN_DIR)/include -o $@ $< $(LIBS)

gcc-record-ld.so: src/gcc-record-ld.c src/sha1file.h src/record-write.h
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< $(LIBS)

test: all
	cd tests && rm -f *.gcc-record* hello.o hello && \
	$(CC) -fplugin=../gcc_record.so -c hello.c && \
	$(CC) -Wl,-plugin=../gcc-record-ld.so -o hello hello.o && \
	echo "== hello.c.gcc-record" && cat hello.c.gcc-record && \
	echo "== hello.gcc-record" && cat hello.gcc-record

clean:
	rm -f *.so tests/*.gcc-record* tests/hello.o tests/hello

.PHONY: all test clean
