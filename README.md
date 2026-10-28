# gcc-record
GCC plugin to record what is being done...

## Build

Requires `gcc-NN-plugin-dev`, `binutils-dev`, `libssl-dev`, `zlib1g-dev` and `libzstd-dev`.

    make        # builds gcc_record.so and gcc-record-ld.so
    make test

## Usage

    gcc -fplugin=./gcc_record.so -c hello.c                  # -> hello.c.gcc-record
    gcc -Wl,-plugin=./gcc-record-ld.so -o hello hello.o      # -> hello.gcc-record

Both together:

    gcc -fplugin=./gcc_record.so -Wl,-plugin=./gcc-record-ld.so -o hello hello.c

write the record files into `DIR`.

Compression:

| | compiler plugin | linker plugin | output |
|---|---|---|---|
| gzip | `-fplugin-arg-gcc_record-z` | `-Wl,-plugin-opt=-z` | `*.gcc-record.gz` |
| zstd | `-fplugin-arg-gcc_record-Z` | `-Wl,-plugin-opt=-Z` | `*.gcc-record.zst` |

