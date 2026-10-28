/* Compute the SHA-1 of a file's contents as a hex string.  */

#ifndef GCC_RECORD_SHA1FILE_H
#define GCC_RECORD_SHA1FILE_H

#include <openssl/evp.h>
#include <stdio.h>

/* Writes 40 hex digits plus NUL into HEX.  Returns 0 on success, -1 on
   error (HEX is then an empty string).  */
static int
sha1_file (const char *name, char hex[41])
{
  unsigned char buf[65536], md[EVP_MAX_MD_SIZE];
  unsigned int md_len = 0, i;
  size_t n;
  int ok;
  FILE *f;
  EVP_MD_CTX *ctx;

  hex[0] = '\0';
  f = fopen (name, "rb");
  if (!f)
    return -1;
  ctx = EVP_MD_CTX_new ();
  ok = ctx && EVP_DigestInit_ex (ctx, EVP_sha1 (), NULL);
  while (ok && (n = fread (buf, 1, sizeof buf, f)) > 0)
    ok = EVP_DigestUpdate (ctx, buf, n);
  ok = ok && !ferror (f) && EVP_DigestFinal_ex (ctx, md, &md_len);
  EVP_MD_CTX_free (ctx);
  fclose (f);
  if (!ok)
    return -1;
  for (i = 0; i < md_len; i++)
    snprintf (hex + 2 * i, 3, "%02x", md[i]);
  return 0;
}

#endif
