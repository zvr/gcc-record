# FOSDEM 2027 — GCC/GNU Toolchain devroom

## Full submission

### Title

GCC Record: Embedding Build Inputs in ELF Binaries

### Abstract

Knowing which files went into a compiled program can make build outputs easier to audit, debug, and understand. gcc-record is a GCC compiler plugin, linker plugin, and wrapper that records the files read during compilation and linking. For each file, it stores the path and SHA-1 digest in a non-allocated, read-only ELF section named `files_read`. As object files are linked, the linker plugin combines and de-duplicates their records with files read during the link itself, including libraries and startup files; the wrapper installs the merged record in the final executable. Records can optionally be gzip- or zstd-compressed.

Because the metadata travels inside the object or executable, it remains available alongside the artifact rather than depending on a separate sidecar file. The section can be extracted with standard binutils tools for inspection or downstream analysis. This talk will demonstrate the workflow and explain how GCC and linker plugins cooperate to capture and merge input records. We will discuss how this lightweight provenance can help developers investigate unexpected dependencies and inspect what a build consumed, as well as the design trade-offs and limitations of recording file paths and hashes.

## Short version

### Title

GCC Record: Embedding Build Inputs in ELF Binaries

### Abstract

gcc-record embeds information about the files used during compilation and linking directly in ELF object files and executables. By keeping this record with the build artifact and combining it across the build, gcc-record makes it easier to inspect build inputs, investigate dependencies, and understand how a program was produced.