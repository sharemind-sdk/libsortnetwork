/**
 * libsortnetwork - src/sn-cut.c
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

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif
#ifdef _XOPEN_SOURCE
# define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "sn_network.h"

static void exit_usage (const char *name)
{
  printf ("Usage: %s <num>[:min|max] [<num>[:min|max] ...]\n", name);
  exit (EXIT_FAILURE);
}

static int do_cut (sn_network_t *n, int defs_num, char **defs)
{
  int mask[SN_NETWORK_INPUT_NUM (n)];
  int status;
  int i;

  memset (mask, 0, sizeof (mask));
  for (i = 0; i < defs_num; i++)
  {
    char *endptr = NULL;
    int pos;
    int val = 1;

    pos = (int) strtol (defs[i], &endptr, /* base = */ 0);
    if (strcasecmp (":min", endptr) == 0)
      val = -1;

    if ((pos < 0) || (pos >= SN_NETWORK_INPUT_NUM (n)))
    {
      fprintf (stderr, "Invalid line number: %i\n", pos);
      return (-1);
    }

    mask[pos] = val;
  }

  status = sn_network_cut (n, mask);
  if (status != 0)
  {
    fprintf (stderr, "sn_network_cut failed with status %i.\n", status);
    return (-1);
  }

  return (0);
} /* }}} int do_cut */

int main (int argc, char **argv)
{
  sn_network_t *n;

  if (argc < 2)
    exit_usage (argv[0]);

  n = sn_network_read (stdin);
  if (n == NULL)
  {
    fprintf (stderr, "Unable to read network from STDIN.\n");
    exit (EXIT_FAILURE);
  }

  do_cut (n, argc - 1, argv + 1);
  sn_network_compress (n);

  sn_network_write (n, stdout);

  exit (EXIT_SUCCESS);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
