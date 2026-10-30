#!/bin/sh
# Builds tests/hello from hello.c + greet.c with each compression mode and
# checks the "files_read" sections.
set -e
cd "$(dirname "$0")"
GR=../gcc-record
cc -O2 -o unzstd unzstd.c -lzstd

show () # file
{
  objcopy --dump-section files_read=sec.bin "$1" /dev/null 2>/dev/null \
    || cp /dev/null sec.bin
  case $(od -An -tx1 -N4 sec.bin | tr -d ' ') in
    1f8b*) zcat sec.bin ;;
    28b52ffd) ./unzstd sec.bin ;;
    *) cat sec.bin ;;
  esac
  rm -f sec.bin
}

for mode in "" --record=gz --record=zstd; do
  echo "######## mode: ${mode:-plain}"
  rm -f hello.o greet.o hello
  $GR $mode -c hello.c -o hello.o
  $GR $mode -c greet.c -o greet.o
  $GR $mode -o hello hello.o greet.o
  readelf -S -W hello | grep files_read
  echo "== hello.o"; show hello.o | head -3
  echo "== greet.o"; show greet.o | head -3
  echo "== hello"; show hello > list.txt; cat list.txt
  for f in hello.c greet.c hello.o greet.o /usr/include/string.h; do
    grep -q "^$f	" list.txt || { echo "MISSING $f"; exit 1; }
  done
  [ "$(sort list.txt | uniq -d)" = "" ] || { echo "DUPLICATES"; exit 1; }
  rm -f list.txt
  ./hello >/dev/null
done
echo "ALL TESTS PASSED"
