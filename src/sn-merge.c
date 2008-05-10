/**
 * collectd - src/sn-merge.c
 * Copyright (C) 2008  Florian octo Forster
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
 *   Florian octo Forster <octo at verplant.org>
 **/

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>
#include <stdio.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s <file0> <file1>\n", name);
  exit (1);
} /* void exit_usage */

int main (int argc, char **argv)
{
  sn_network_t *n0;
  sn_network_t *n1;
  sn_network_t *n;

  if (argc != 3)
    exit_usage (argv[0]);

  n0 = sn_network_read_file (argv[1]);
  if (n0 == NULL)
  {
    printf ("n0 == NULL\n");
    return (1);
  }

  n1 = sn_network_read_file (argv[2]);
  if (n1 == NULL)
  {
    printf ("n1 == NULL\n");
    return (1);
  }

  n = sn_network_combine (n0, n1);
  sn_network_destroy (n0);
  sn_network_destroy (n1);

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
