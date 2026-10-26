/* gcc-record: GCC plugin recording the files read while compiling.  */

#include "gcc-plugin.h"
#include "plugin-version.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

int plugin_is_GPL_compatible;

static std::vector<std::string> recorded;
static std::set<std::string> seen;
static std::string output_dir;

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
on_finish (void *, void *)
{
  if (!main_input_filename)
    return;

  std::string out;
  if (!output_dir.empty ())
    {
      const char *base = strrchr (main_input_filename, '/');
      out = output_dir + "/" + (base ? base + 1 : main_input_filename);
    }
  else
    out = main_input_filename;
  out += ".gcc-record";

  FILE *f = fopen (out.c_str (), "w");
  if (!f)
    {
      fprintf (stderr, "gcc-record: cannot write %s\n", out.c_str ());
      return;
    }
  fprintf (f, "%s\n", main_input_filename);
  for (const std::string &s : recorded)
    if (s != main_input_filename)
      fprintf (f, "%s\n", s.c_str ());
  fclose (f);
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
    if (!strcmp (info->argv[i].key, "dir") && info->argv[i].value)
      output_dir = info->argv[i].value;

  register_callback (info->base_name, PLUGIN_INCLUDE_FILE, on_include_file,
		     NULL);
  register_callback (info->base_name, PLUGIN_FINISH, on_finish, NULL);
  return 0;
}
