/* Write a record buffer to disk, optionally compressed.  */

#ifndef GCC_RECORD_WRITE_H
#define GCC_RECORD_WRITE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <zstd.h>

enum record_compress
{
  RECORD_PLAIN,
  RECORD_GZIP,
  RECORD_ZSTD
};

static const char *
record_suffix (enum record_compress mode)
{
  switch (mode)
    {
    case RECORD_GZIP:
      return ".gz";
    case RECORD_ZSTD:
      return ".zst";
    default:
      return "";
    }
}

/* Writes LEN bytes of DATA to PATH.  Returns 0 on success, -1 on error.  */
static int
record_write (const char *path, const char *data, size_t len,
	      enum record_compress mode)
{
  if (mode == RECORD_GZIP)
    {
      gzFile gz = gzopen (path, "wb9");
      int ok;

      if (!gz)
	return -1;
      ok = len == 0 || gzwrite (gz, data, (unsigned) len) == (int) len;
      return (gzclose (gz) == Z_OK && ok) ? 0 : -1;
    }
  else
    {
      FILE *f = fopen (path, "wb");
      int ok;

      if (!f)
	return -1;
      if (mode == RECORD_ZSTD)
	{
	  size_t cap = ZSTD_compressBound (len), n;
	  void *buf = malloc (cap ? cap : 1);

	  if (!buf)
	    {
	      fclose (f);
	      return -1;
	    }
	  n = ZSTD_compress (buf, cap, data, len, ZSTD_maxCLevel ());
	  ok = !ZSTD_isError (n) && fwrite (buf, 1, n, f) == n;
	  free (buf);
	}
      else
	ok = fwrite (data, 1, len, f) == len;
      return (fclose (f) == 0 && ok) ? 0 : -1;
    }
}

#endif
