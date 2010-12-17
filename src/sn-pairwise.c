/**
 * libsortnetwork - src/sn-pairwise.c
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

#include "sn_network.h"

int main (int argc, char **argv)
{
  sn_network_t *n;
  size_t inputs_num;

  if (argc != 2)
  {
    printf ("Usage: %s <num inputs>\n", argv[0]);
    return (0);
  }

  inputs_num = (size_t) atoi (argv[1]);
  if (inputs_num < 2)
  {
    fprintf (stderr, "Invalid number of inputs: %zu\n", inputs_num);
    return (1);
  }

  n = sn_network_create_pairwise (inputs_num);
  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
