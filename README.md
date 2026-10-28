# gcc-record
GCC plugin to record what is being done...

## Build

Requires `gcc-NN-plugin-dev`, `binutils-dev` and `libssl-dev`.

    make        # builds gcc-record.so and gcc-record-ld.so
    make test

## Usage

    gcc -fplugin=./gcc-record.so -c hello.c                  # -> hello.c.gcc-record
    gcc -Wl,-plugin=./gcc-record-ld.so -o hello hello.o      # -> hello.gcc-record

Both together:

    gcc -fplugin=./gcc-record.so -Wl,-plugin=./gcc-record-ld.so -o hello hello.c

Options: `-fplugin-arg-gcc-record-dir=DIR` / `-Wl,-plugin-opt=dir=DIR` write the record files into `DIR`.

