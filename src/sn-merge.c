/**
 * libsortnetwork - src/sn-merge.c
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
#include <unistd.h>

#include "sn_network.h"

static _Bool use_bitonic = 0;
static const char *file0 = NULL;
static const char *file1 = NULL;

static void exit_usage (void) /* {{{ */
{
  printf ("sn-merge [options] <file0> <file1>\n"
      "\n"
      "Options:\n"
      "  -b        Use the bitonic merger.\n"
      "  -o        Use the odd-even merger. (default)\n"
      "  -h        Display this help and exit.\n"
      "\n");
  exit (EXIT_FAILURE);
} /* }}} void exit_usage */

static int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "boh")) != -1)
  {
    switch (option)
    {
      case 'b':
	use_bitonic = 1;
	break;

      case 'o':
	use_bitonic = 0;
	break;

      case 'h':
      default:
	exit_usage ();
    }
  }

  if ((argc - optind) != 2)
    exit_usage ();

  file0 = argv[optind];
  file1 = argv[optind + 1];

  if ((file0 == NULL) || (file1 == NULL))
    exit_usage ();

  return (0);
} /* }}} int read_options */

static _Bool is_power_of_two (int n)
{
  if (n < 1)
    return (0);
  else if ((n == 1) || (n == 2))
    return (1);
  else if ((n % 2) != 0)
    return (0);
  else
    return (is_power_of_two (n >> 1));
} /* _Bool is_power_of_two */

int main (int argc, char **argv)
{
  sn_network_t *n0;
  sn_network_t *n1;
  sn_network_t *n;

  read_options (argc, argv);

  if (strcmp ("-", file0) == 0)
    n0 = sn_network_read (stdin);
  else
    n0 = sn_network_read_file (file0);
  if (n0 == NULL)
  {
    fprintf (stderr, "Unable to read first network.\n");
    exit (EXIT_FAILURE);
  }

  if (strcmp ("-", file1) == 0)
  {
    if (strcmp ("-", file0) == 0)
      n1 = sn_network_clone (n0);
    else
      n1 = sn_network_read (stdin);
  }
  else
    n1 = sn_network_read_file (file1);
  if (n1 == NULL)
  {
    fprintf (stderr, "Unable to read second network.\n");
    exit (EXIT_FAILURE);
  }

  if (use_bitonic)
    n = sn_network_combine_bitonic_merge (n0, n1);
  else
    n = sn_network_combine_odd_even_merge (n0, n1);

  if (n == NULL)
  {
    fprintf (stderr, "Combining the networks faild.\n");
    exit (EXIT_FAILURE);
  }

  sn_network_destroy (n0);
  sn_network_destroy (n1);

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
