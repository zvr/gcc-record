# gcc-record
GCC plugin to record what is being done...

## Build

Requires `gcc-NN-plugin-dev`, `binutils-dev`, `libssl-dev`, `zlib1g-dev` and `libzstd-dev`.

    make        # builds gcc_record.so and gcc-record-ld.so
    make test

## Usage

The `gcc-record` script is a drop-in replacement for `gcc` that loads both plugins:

    ./gcc-record -c src/hello.c -o out/hello.o    # -> out/hello.c.gcc-record
    ./gcc-record -o hello hello.o                  # -> hello.gcc-record
    ./gcc-record --record=gz ...                   # gzip-compressed records
    ./gcc-record --record=zstd ...                 # zstd-compressed records

All other arguments (including `-z`/`-Z`) go to `gcc` unchanged.
`GCC_RECORD_PLUGIN_DIR` the plugin location (default: the script's directory).

Manual use of the plugins:

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

