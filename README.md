# gcc-record
GCC plugin to record what is being done...

## Build

Requires `gcc-NN-plugin-dev`, `binutils-dev`, `libssl-dev`, `zlib1g-dev` and `libzstd-dev`.

    make        # builds gcc_record.so and gcc-record-ld.so
    make test

## Usage

The `gcc-record` script is a drop-in replacement for `gcc`:

    ./gcc-record -c hello.c -o hello.o
    ./gcc-record -c greet.c -o greet.o
    ./gcc-record -o hello hello.o greet.o
    ./gcc-record --record=gz ...       # gzip-compress the section
    ./gcc-record --record=zstd ...     # zstd-compress the section

The list of files read (`filename<TAB>sha1` per line) is stored in a
non-allocated, read-only ELF section named `files_read`:

- each object file gets the files read while compiling it;
- the linked output gets a single `files_read` section holding the
  de-duplicated union of the sections of all its inputs plus every file
  read by the linker (objects, libraries, startup files).

Inspect it with:

    objcopy --dump-section files_read=list.txt hello /dev/null

All other arguments (including `-z`/`-Z`) go to `gcc` unchanged.
`GCC_RECORD_CC` selects the compiler (default `gcc`), `GCC_RECORD_OBJCOPY`
the objcopy (default `objcopy`) and `GCC_RECORD_PLUGIN_DIR` the plugin
location (default: the script's directory).

## Plugins

- `gcc_record.so` (`-fplugin=`) emits the `files_read` section directly into
  each compiled object. Options: `-fplugin-arg-gcc_record-z` / `-Z`.
- `gcc-record-ld.so` (`-Wl,-plugin=`) merges the inputs' sections (plain, gzip
  or zstd) with the linker's own inputs and writes the result to
  `-plugin-opt=out=PATH` (default `OUTPUT.files_read`). Options:
  `-plugin-opt=-z` / `-Z`. A linker plugin cannot replace an output section,
  so the script installs this file into the output with `objcopy`.
