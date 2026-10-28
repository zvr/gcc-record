/* gcc-record-ld: linker plugin recording the files read while linking.  */

#include <stdint.h>
#include <plugin-api.h>

#include "sha1file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **recorded;
static size_t n_recorded, cap_recorded;
static const char *output_name = "a.out";
static const char *output_dir;
static ld_plugin_message message;

static void
record_file (const char *name)
{
  size_t i;

  if (!name || !*name)
    return;
  for (i = 0; i < n_recorded; i++)
    if (!strcmp (recorded[i], name))
      return;
  if (n_recorded == cap_recorded)
    {
      cap_recorded = cap_recorded ? cap_recorded * 2 : 64;
      recorded = realloc (recorded, cap_recorded * sizeof *recorded);
      if (!recorded)
	abort ();
    }
  recorded[n_recorded++] = strdup (name);
}

static enum ld_plugin_status
claim_file (const struct ld_plugin_input_file *file, int *claimed)
{
  *claimed = 0;
  record_file (file->name);
  return LDPS_OK;
}

static enum ld_plugin_status
cleanup (void)
{
  const char *base = output_name;
  char *out;
  FILE *f;
  size_t i;

  if (output_dir)
    {
      const char *slash = strrchr (output_name, '/');
      base = slash ? slash + 1 : output_name;
    }
  out = malloc ((output_dir ? strlen (output_dir) + 1 : 0) + strlen (base)
		+ sizeof ".gcc-record");
  if (!out)
    return LDPS_ERR;
  sprintf (out, "%s%s%s.gcc-record", output_dir ? output_dir : "",
	   output_dir ? "/" : "", base);

  f = fopen (out, "w");
  if (!f)
    {
      if (message)
	message (LDPL_WARNING, "gcc-record: cannot write %s", out);
      free (out);
      return LDPS_OK;
    }
  for (i = 0; i < n_recorded; i++)
    {
      char hex[41];
      sha1_file (recorded[i], hex);
      fprintf (f, "%s\t%s\n", recorded[i], hex);
      free (recorded[i]);
    }
  fclose (f);
  free (out);
  free (recorded);
  recorded = NULL;
  n_recorded = cap_recorded = 0;
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
	if (!strncmp (tv->tv_u.tv_string, "dir=", 4))
	  output_dir = tv->tv_u.tv_string + 4;
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
