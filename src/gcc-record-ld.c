/* gcc-record-ld: linker plugin recording the files read while linking.

   Collects every input file seen by the linker, together with the
   "files_read" sections already present in input objects (which may be
   plain, gzip or zstd encoded), and writes the merged, de-duplicated list
   (one "filename<TAB>sha1" per line, optionally compressed) to a file.  The
   gcc-record script then stores that file as the "files_read" section of the
   output, since a linker plugin cannot replace an output section itself.

   Options (-plugin-opt=...):
     out=PATH   where to write the merged list (default: OUTPUT.files_read)
     -z / -Z    gzip / zstd compress the written list  */

#define _GNU_SOURCE
#include <stdint.h>
#include <plugin-api.h>

#include "sha1file.h"
#include "record-compress.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SECTION_NAME "files_read"

static char **lines;
static size_t n_lines, cap_lines;
static const char *output_name = "a.out";
static const char *out_path;
static enum record_compress compress_mode = RECORD_PLAIN;
static ld_plugin_message message;

static void
add_line (const char *line, size_t len)
{
  size_t i;

  if (len == 0)
    return;
  for (i = 0; i < n_lines; i++)
    if (strlen (lines[i]) == len && !memcmp (lines[i], line, len))
      return;
  if (n_lines == cap_lines)
    {
      cap_lines = cap_lines ? cap_lines * 2 : 64;
      lines = realloc (lines, cap_lines * sizeof *lines);
      if (!lines)
	abort ();
    }
  lines[n_lines++] = strndup (line, len);
}

static void
add_text (const char *text, size_t len)
{
  while (len)
    {
      const char *nl = memchr (text, '\n', len);
      size_t l = nl ? (size_t) (nl - text) : len;

      add_line (text, l);
      if (!nl)
	break;
      len -= l + 1;
      text = nl + 1;
    }
}

/* Decodes a sequence of plain / gzip / zstd chunks (concatenated sections
   from several objects, e.g. after "ld -r") and adds their lines.  */
static void
add_section_data (const unsigned char *p, size_t len)
{
  while (len)
    {
      if (len >= 2 && p[0] == 0x1f && p[1] == 0x8b)
	{
	  z_stream zs;
	  unsigned char out[65536];
	  int rc;

	  memset (&zs, 0, sizeof zs);
	  if (inflateInit2 (&zs, 15 + 16) != Z_OK)
	    return;
	  zs.next_in = (Bytef *) p;
	  zs.avail_in = (uInt) len;
	  do
	    {
	      zs.next_out = out;
	      zs.avail_out = sizeof out;
	      rc = inflate (&zs, Z_NO_FLUSH);
	      /* Lines may straddle buffer boundaries; collect via memstream. */
	      if (rc == Z_OK || rc == Z_STREAM_END)
		{
		  static char *acc;
		  static size_t acc_len;
		  size_t n = sizeof out - zs.avail_out;

		  acc = realloc (acc, acc_len + n + 1);
		  memcpy (acc + acc_len, out, n);
		  acc_len += n;
		  if (rc == Z_STREAM_END)
		    {
		      add_text (acc, acc_len);
		      free (acc);
		      acc = NULL;
		      acc_len = 0;
		    }
		}
	    }
	  while (rc == Z_OK);
	  p += zs.total_in;
	  len -= zs.total_in;
	  inflateEnd (&zs);
	  if (rc != Z_STREAM_END)
	    return;
	}
      else if (len >= 4 && p[0] == 0x28 && p[1] == 0xb5 && p[2] == 0x2f
	       && p[3] == 0xfd)
	{
	  size_t fsz = ZSTD_findFrameCompressedSize (p, len);
	  unsigned long long dsz = ZSTD_getFrameContentSize (p, len);
	  char *out;
	  size_t n;

	  if (ZSTD_isError (fsz) || dsz == ZSTD_CONTENTSIZE_UNKNOWN
	      || dsz == ZSTD_CONTENTSIZE_ERROR)
	    return;
	  out = malloc (dsz ? dsz : 1);
	  if (!out)
	    return;
	  n = ZSTD_decompress (out, dsz, p, fsz);
	  if (!ZSTD_isError (n))
	    add_text (out, n);
	  free (out);
	  p += fsz;
	  len -= fsz;
	}
      else
	{
	  add_text ((const char *) p, len);
	  return;
	}
    }
}

static int
read_at (int fd, off_t off, void *buf, size_t len)
{
  return pread (fd, buf, len, off) == (ssize_t) len ? 0 : -1;
}

#define DEFINE_SCAN(BITS)                                                     \
  static void scan_elf##BITS (int fd, off_t base)                             \
  {                                                                           \
    Elf##BITS##_Ehdr eh;                                                      \
    Elf##BITS##_Shdr *sh, *strsh;                                             \
    char *names;                                                              \
    unsigned i;                                                               \
									      \
    if (read_at (fd, base, &eh, sizeof eh) || !eh.e_shoff                     \
	|| eh.e_shentsize != sizeof (Elf##BITS##_Shdr)                        \
	|| eh.e_shstrndx >= eh.e_shnum)                                       \
      return;                                                                 \
    sh = malloc (eh.e_shnum * sizeof *sh);                                    \
    if (!sh)                                                                  \
      return;                                                                 \
    if (read_at (fd, base + eh.e_shoff, sh, eh.e_shnum * sizeof *sh))         \
      {                                                                       \
	free (sh);                                                            \
	return;                                                               \
      }                                                                       \
    strsh = &sh[eh.e_shstrndx];                                               \
    names = malloc (strsh->sh_size + 1);                                      \
    if (names && !read_at (fd, base + strsh->sh_offset, names,                \
			   strsh->sh_size))                                   \
      {                                                                       \
	names[strsh->sh_size] = '\0';                                         \
	for (i = 0; i < eh.e_shnum; i++)                                      \
	  if (sh[i].sh_name < strsh->sh_size                                  \
	      && !strcmp (names + sh[i].sh_name, SECTION_NAME)                \
	      && sh[i].sh_type == SHT_PROGBITS && sh[i].sh_size)              \
	    {                                                                 \
	      unsigned char *d = malloc (sh[i].sh_size);                      \
	      if (d && !read_at (fd, base + sh[i].sh_offset, d,               \
				 sh[i].sh_size))                              \
		add_section_data (d, sh[i].sh_size);                          \
	      free (d);                                                       \
	    }                                                                 \
      }                                                                       \
    free (names);                                                             \
    free (sh);                                                                \
  }

DEFINE_SCAN (32)
DEFINE_SCAN (64)

static void
scan_input (int fd, off_t base)
{
  unsigned char ident[EI_NIDENT];

  if (read_at (fd, base, ident, sizeof ident)
      || memcmp (ident, ELFMAG, SELFMAG))
    return;
  if (ident[EI_CLASS] == ELFCLASS64)
    scan_elf64 (fd, base);
  else if (ident[EI_CLASS] == ELFCLASS32)
    scan_elf32 (fd, base);
}

static void
record_input (const char *name)
{
  char hex[41];
  char *line;

  if (!name || !*name)
    return;
  sha1_file (name, hex);
  if (asprintf (&line, "%s\t%s", name, hex) < 0)
    return;
  add_line (line, strlen (line));
  free (line);
}

static enum ld_plugin_status
claim_file (const struct ld_plugin_input_file *file, int *claimed)
{
  *claimed = 0;
  record_input (file->name);
  scan_input (file->fd, file->offset);
  return LDPS_OK;
}

static enum ld_plugin_status
cleanup (void)
{
  char *data = NULL, *path = NULL;
  size_t data_len = 0, out_len, i;
  unsigned char *buf;
  FILE *f;

  f = open_memstream (&data, &data_len);
  if (!f)
    return LDPS_ERR;
  for (i = 0; i < n_lines; i++)
    {
      fprintf (f, "%s\n", lines[i]);
      free (lines[i]);
    }
  fclose (f);
  free (lines);
  lines = NULL;
  n_lines = cap_lines = 0;

  buf = record_compress_buf (data, data_len, compress_mode, &out_len);
  free (data);
  if (!buf)
    return LDPS_ERR;

  if (!out_path && asprintf (&path, "%s." SECTION_NAME, output_name) < 0)
    path = NULL;
  f = fopen (out_path ? out_path : path, "wb");
  if (!f || fwrite (buf, 1, out_len, f) != out_len || fclose (f))
    {
      if (message)
	message (LDPL_WARNING, "gcc-record: cannot write %s",
		 out_path ? out_path : path);
    }
  record_free (buf);
  free (path);
  return LDPS_OK;
}

enum ld_plugin_status
onload (struct ld_plugin_tv *tv)
{
  ld_plugin_register_claim_file reg_claim = NULL;
  ld_plugin_register_cleanup reg_cleanup = NULL;

  for (; tv->tv_tag != LDPT_NULL; tv++)
    switch (tv->tv_tag)
      {
      case LDPT_REGISTER_CLAIM_FILE_HOOK:
	reg_claim = tv->tv_u.tv_register_claim_file;
	break;
      case LDPT_REGISTER_CLEANUP_HOOK:
	reg_cleanup = tv->tv_u.tv_register_cleanup;
	break;
      case LDPT_OUTPUT_NAME:
	output_name = tv->tv_u.tv_string;
	break;
      case LDPT_MESSAGE:
	message = tv->tv_u.tv_message;
	break;
      case LDPT_OPTION:
	if (!strncmp (tv->tv_u.tv_string, "out=", 4))
	  out_path = tv->tv_u.tv_string + 4;
	else if (!strcmp (tv->tv_u.tv_string, "-z")
		 || !strcmp (tv->tv_u.tv_string, "z"))
	  compress_mode = RECORD_GZIP;
	else if (!strcmp (tv->tv_u.tv_string, "-Z")
		 || !strcmp (tv->tv_u.tv_string, "Z"))
	  compress_mode = RECORD_ZSTD;
	break;
      default:
	break;
      }

  if (!reg_claim || !reg_cleanup)
    return LDPS_ERR;
  reg_claim (claim_file);
  reg_cleanup (cleanup);
  return LDPS_OK;
}
