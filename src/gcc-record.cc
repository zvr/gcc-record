/* gcc-record: GCC plugin recording the files read while compiling.

   The list (one "filename<TAB>sha1" per line, optionally compressed) is
   emitted into a non-allocated, read-only "files_read" section of the
   generated object file.  */

/* Must precede GCC headers, which poison malloc.  */
#include "record-compress.h"

#include "gcc-plugin.h"
#include "plugin-version.h"
#include "sha1file.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

int plugin_is_GPL_compatible;

extern FILE *asm_out_file;

static std::vector<std::string> recorded;
static std::set<std::string> seen;
static enum record_compress compress_mode = RECORD_PLAIN;

static void
record_file (const char *name)
{
  if (!name || !*name || *name == '<')
    return;
  if (seen.insert (name).second)
    recorded.push_back (name);
}

static void
on_include_file (void *gcc_data, void *)
{
  record_file ((const char *) gcc_data);
}

static void
on_finish_unit (void *, void *)
{
  if (!main_input_filename || !asm_out_file)
    return;

  std::string data;
  char hex[41];
  sha1_file (main_input_filename, hex);
  data += std::string (main_input_filename) + "\t" + hex + "\n";
  for (const std::string &s : recorded)
    if (s != main_input_filename)
      {
	sha1_file (s.c_str (), hex);
	data += s + "\t" + hex + "\n";
      }

  size_t len;
  unsigned char *buf
    = record_compress_buf (data.data (), data.size (), compress_mode, &len);
  if (!buf)
    {
      fprintf (stderr, "gcc-record: cannot compress record\n");
      return;
    }

  fprintf (asm_out_file, "\t.pushsection files_read,\"\",@progbits\n");
  for (size_t i = 0; i < len; i++)
    fprintf (asm_out_file, "%s%u%s", i % 16 ? "," : "\t.byte ", buf[i],
	     (i % 16 == 15 || i + 1 == len) ? "\n" : "");
  fprintf (asm_out_file, "\t.popsection\n");
  record_free (buf);
}

int
plugin_init (struct plugin_name_args *info, struct plugin_gcc_version *version)
{
  if (!plugin_default_version_check (version, &gcc_version))
    {
      fprintf (stderr, "gcc-record: incompatible GCC version\n");
      return 1;
    }

  for (int i = 0; i < info->argc; i++)
    {
      const char *key = info->argv[i].key;
      if (!strcmp (key, "z"))
	compress_mode = RECORD_GZIP;
      else if (!strcmp (key, "Z"))
	compress_mode = RECORD_ZSTD;
    }

  register_callback (info->base_name, PLUGIN_INCLUDE_FILE, on_include_file,
		     NULL);
  register_callback (info->base_name, PLUGIN_FINISH_UNIT, on_finish_unit,
		     NULL);
  return 0;
}
