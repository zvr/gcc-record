# GCC-record

This is a plugin to GCC.

When installed and enabled in a run, it records the files read by GCC while compiling a file.

For example, when `gcc -c hello.c` is run, it will record all the files read by GCC while compiling `hello.c`, including `hello.c` itself, any included headers, and any other files that GCC reads during the compilation process.
The files are written in a separate file, named after the filename being compiled, with the extension `.gcc-record`. For example, if `hello.c` is compiled, the recorded files will be written to `hello.c.gcc-record`.

When GCC is used to link together multiple object files, the plugin will record all the files read by GCC while linking the object files together. This includes the standard libraries that are usually also linked in the resulting executable. The recorded files will be written to a file named after the output file being generated, with the extension `.gcc-record`. For example, if `gcc -o hello hello.o` is run, the recorded files will be written to `hello.gcc-record`.

## Options

Some options to the plugin may be used to modify its runtime behavior.
