/* In-memory compression helpers shared by gcc-record components.  */

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

/* Compresses LEN bytes of DATA.  Returns a malloc'd buffer (release with
   record_free) and sets *OUT_LEN, or returns NULL on error.  */
static unsigned char *
record_compress_buf (const void *data, size_t len, enum record_compress mode,
		     size_t *out_len)
{
  unsigned char *buf;

  if (mode == RECORD_GZIP)
    {
      z_stream zs;
      size_t cap;

      memset (&zs, 0, sizeof zs);
      if (deflateInit2 (&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 9,
			Z_DEFAULT_STRATEGY) != Z_OK)
	return NULL;
      cap = deflateBound (&zs, len);
      buf = (unsigned char *) malloc (cap);
      if (!buf)
	{
	  deflateEnd (&zs);
	  return NULL;
	}
      zs.next_in = (Bytef *) data;
      zs.avail_in = (uInt) len;
      zs.next_out = buf;
      zs.avail_out = (uInt) cap;
      if (deflate (&zs, Z_FINISH) != Z_STREAM_END)
	{
	  deflateEnd (&zs);
	  free (buf);
	  return NULL;
	}
      *out_len = zs.total_out;
      deflateEnd (&zs);
      return buf;
    }
  if (mode == RECORD_ZSTD)
    {
      size_t cap = ZSTD_compressBound (len), n;

      buf = (unsigned char *) malloc (cap ? cap : 1);
      if (!buf)
	return NULL;
      n = ZSTD_compress (buf, cap, data, len, ZSTD_maxCLevel ());
      if (ZSTD_isError (n))
	{
	  free (buf);
	  return NULL;
	}
      *out_len = n;
      return buf;
    }
  buf = (unsigned char *) malloc (len ? len : 1);
  if (!buf)
    return NULL;
  memcpy (buf, data, len);
  *out_len = len;
  return buf;
}

static void
record_free (void *p)
{
  free (p);
}

#endif
