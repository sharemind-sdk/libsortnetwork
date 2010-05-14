/**
 * libsortnetwork - src/sn-normalize.c
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

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s [file]\n", name);
  exit (1);
}

int main (int argc, char **argv)
{
  sn_network_t *n;
  int status;

  if (argc > 2)
  {
    exit_usage (argv[0]);
  }
  else if (argc == 2)
  {
    if ((strcmp ("-h", argv[1]) == 0)
	|| (strcmp ("--help", argv[1]) == 0)
	|| (strcmp ("-help", argv[1]) == 0))
      exit_usage (argv[0]);

    n = sn_network_read_file (argv[1]);
  }
  else
  {
    n = sn_network_read (stdin);
  }

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  status = sn_network_brute_force_check (n);

  if (status < 0)
  {
    printf ("sn_network_brute_force_check failed with status %i.\n", status);
  }
  else if (status > 0)
  {
    printf ("The network does NOT sort.\n");
  }
  else
  {
    printf ("All tests successful.\n");
  }

  return (status);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
