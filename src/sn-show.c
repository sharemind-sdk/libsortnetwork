/**
 * collectd - src/sn-show.c
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
  FILE *fh = NULL;

  if (argc == 1)
    fh = stdin;
  else if (argc == 2)
    fh = fopen (argv[1], "r");

  if (fh == NULL)
  {
    printf ("fh == NULL!\n");
    return (1);
  }

  n = sn_network_read (fh);

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_show (n);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
