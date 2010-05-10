/**
 * collectd - src/sn-cut.c
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
#include <strings.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s <position> <min|max>\n", name);
  exit (EXIT_FAILURE);
}

int main (int argc, char **argv)
{
  sn_network_t *n;

  int pos = 0;
  enum sn_network_cut_dir_e dir = DIR_MIN;

  if (argc != 3)
    exit_usage (argv[0]);

  pos = atoi (argv[1]);
  if (strcasecmp ("max", argv[2]) == 0)
    dir = DIR_MAX;

  n = sn_network_read (stdin);
  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_cut_at (n, pos, dir);
  sn_network_compress (n);

  sn_network_write (n, stdout);

  exit (EXIT_SUCCESS);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
