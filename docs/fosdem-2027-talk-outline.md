# FOSDEM 2027 — GCC/GNU Toolchain devroom
## GCC Record: Embedding Build Inputs in ELF Binaries

### Slide 1 — Title
- GCC Record: Embedding Build Inputs in ELF Binaries
- FOSDEM 2027 — GCC/GNU Toolchain devroom
- A compiler and linker plugin experiment in build provenance

### Slide 2 — The build-input visibility gap
- A build consumes more than the source file named on the command line
- Compilation reads headers and other files; linking consumes objects, libraries, and startup files
- Once the build finishes, those inputs are not necessarily easy to discover from the output
- Separate logs and sidecar manifests can become detached from the artifact

### Slide 3 — Why this is useful
- Investigate which inputs contributed to a build artifact
- Make build outputs easier to inspect during debugging and review
- Keep a compact input record alongside the object or executable
- Give downstream tooling a simple section to extract and analyze
- Hashes help distinguish recorded file contents; they are not a signature or proof that a build is trustworthy

### Slide 4 — Previous solutions: source identification keywords
- SCCS and CVS supported identification keywords embedded in source text
- Examples include SCCS `%W%` / `%G%` and CVS `$Id$` / `$Revision$`
- These are useful for identifying individual source files and their revisions
- They depend on keywords being present in source and expanded by the version-control workflow
- They do not automatically provide a consolidated record of the files actually read across compilation and linking

### Slide 5 — What gcc-record does
- Records the files observed during compilation and linking
- Stores a path and SHA-1 digest for each recorded file
- Embeds the record in a non-allocated, read-only ELF section named `files_read`
- Supports plain text, gzip, and zstd section contents
- Provides a wrapper that loads the compiler and linker plugins automatically

### Slide 6 — A simple workflow
- Build the plugins with `make`
- Compile and link using `gcc-record` in place of `gcc`
- Example:
  - `./gcc-record -c hello.c -o hello.o`
  - `./gcc-record -o hello hello.o`
- Optional record compression:
  - `--record=gz`
  - `--record=zstd`
- Other compiler arguments, including GCC's `-z` and `-Z`, are passed through

### Slide 7 — Compilation: record the translation unit
- The GCC plugin observes included files and the main input file
- At the end of compilation, it creates the `files_read` section in the object
- Each record is one `path<TAB>sha1` line
- Section flags make it read-only and not loaded as part of the program image

### Slide 8 — Linking: collect and merge
- The linker plugin sees the linker's input files
- It records their paths and digests, and scans ELF inputs for existing `files_read` sections
- Existing sections can be plain, gzip-compressed, or zstd-compressed
- The plugin combines records and removes duplicate lines
- The linker plugin writes a merged temporary record; the wrapper uses `objcopy` to install one final section

### Slide 9 — Inspecting the result
- Confirm the section exists with `readelf -S -W hello`
- Extract it with `objcopy --dump-section files_read=inputs.txt hello /dev/null`
- The extracted list can be reviewed or consumed by downstream scripts
- The record stays with the object or executable when copied as a file

### Slide 10 — Demonstration
- Compile two source files separately with `gcc-record`
- Show the `files_read` section in each object
- Link the objects and inspect the resulting executable's section
- Check that the final list includes source/header files, object files, and linker inputs
- Repeat with gzip and zstd modes; run the generated program

### Slide 11 — Scope and limitations
- The current implementation targets ELF and GCC's plugin interface
- It records files observed by the compiler and linker plugins; it is not a full hermetic-build system
- Paths depend on the build environment and may not be portable
- Consumers should treat the section as build metadata, not as a security attestation
- Compression reduces section size but requires a compatible decoder to inspect the contents

### Slide 12 — Wrap-up and discussion
- gcc-record puts a useful record of observed build inputs inside ELF artifacts
- Compiler and linker plugins capture per-object records and combine them at link time
- The result is directly inspectable with familiar binutils tools
- Discussion: what additional metadata or integration would make this more useful in real toolchains?
