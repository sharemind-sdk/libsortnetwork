/**
 * libsortnetwork - src/sn-show.c
 * Copyright (C) 2008-2010  Florian octo Forster
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Authors:
 *   Florian octo Forster <ff at octo.it>
 **/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sn_network.h"

static int show_fh (FILE *fh) /* {{{ */
{
  sn_network_t *n;

  if (fh == NULL)
    return (EINVAL);

  n = sn_network_read (fh);
  if (n == NULL)
  {
    fprintf (stderr, "Parsing comparator network failed.\n");
    return (EINVAL);
  }

  sn_network_show (n);

  sn_network_destroy (n);

  return (0);
} /* }}} int show_fh */

static int show_file (const char *file) /* {{{ */
{
  FILE *fh;
  int status;

  if (file == NULL)
    return (EINVAL);

  fh = fopen (file, "r");
  if (fh == NULL)
  {
    fprintf (stderr, "Opening file \"%s\" failed: %s\n",
        file, strerror (errno));
    return (errno);
  }

  status = show_fh (fh);

  fclose (fh);
  return (status);
} /* }}} int show_file */

int main (int argc, char **argv)
{
  if (argc == 1)
  {
    show_fh (stdin);
  }
  else
  {
    int i;
    for (i = 1; i < argc; i++)
    {
      if (i > 1)
        puts ("\n");

      if (argc > 2)
        printf ("=== %s ===\n\n", argv[i]);

      show_file (argv[i]);
    }
  }

  exit (EXIT_SUCCESS);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
