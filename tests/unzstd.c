/* Test helper: decompress a file of concatenated zstd frames to stdout.  */

#include <stdio.h>
#include <stdlib.h>
#include <zstd.h>

int
main (int argc, char **argv)
{
  static char in[1 << 22];
  FILE *f;
  size_t n, off = 0;

  if (argc != 2 || !(f = fopen (argv[1], "rb")))
    return 2;
  n = fread (in, 1, sizeof in, f);
  fclose (f);
  while (off < n)
    {
      size_t fsz = ZSTD_findFrameCompressedSize (in + off, n - off);
      unsigned long long dsz = ZSTD_getFrameContentSize (in + off, n - off);
      char *out;
      size_t m;

      if (ZSTD_isError (fsz) || dsz == ZSTD_CONTENTSIZE_UNKNOWN
	  || dsz == ZSTD_CONTENTSIZE_ERROR || !(out = malloc (dsz + 1)))
	return 1;
      m = ZSTD_decompress (out, dsz, in + off, fsz);
      if (ZSTD_isError (m))
	return 1;
      fwrite (out, 1, m, stdout);
      free (out);
      off += fsz;
    }
  return 0;
}
