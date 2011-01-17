/**
 * libsortnetwork - src/sn-count-cuts.c
 * Copyright (C) 2008-2011  Florian octo Forster
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
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "sn_network.h"
#include "sn_random.h"
#include "sn_hashtable.h"

static int cuts_num = 0;
static uint64_t iterations_num = 1000000;
static char *input_file = NULL;

static _Bool exit_after_collision = 0;

static sn_hashtable_t *hashtable;

static double possible_cuts (int inputs_num) /* {{{ */
{
  double n = (double) inputs_num;
  double k = (double) cuts_num;

  double tmp = lgamma (1.0 + n)
    - (lgamma (1.0 + k) + lgamma (1.0 + n - k));

  return (pow (2.0, k) * exp (tmp));
} /* }}} double possible_cuts */

static double estimate_reachable_networks (sn_network_t *n) /* {{{ */
{
  double cuts = possible_cuts (SN_NETWORK_INPUT_NUM (n));
  double pct = sn_hashtable_get_networks_pct (hashtable) / 100.0;

  return (cuts * pct);
} /* }}} double estimate_reachable_networks */

static void exit_usage (void) /* {{{ */
{
  printf ("sn-count-cuts [options] <file0> <file1>\n"
      "\n"
      "Options:\n"
      "  -1        Exit after the first collision has been detected.\n"
      "  -c        Number of cuts to perform.\n"
      "  -n        Maximum number of cuts to perform.\n"
      "  -h        Display this help and exit.\n"
      "\n");
  exit (EXIT_FAILURE);
} /* }}} void exit_usage */

static int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "1c:n:h")) != -1)
  {
    switch (option)
    {
      case '1':
	exit_after_collision = 1;
	break;

      case 'c':
      {
	int tmp = atoi (optarg);
	if (tmp <= 0)
	{
	  fprintf (stderr, "Not a valid number of cuts: %s\n", optarg);
	  exit_usage ();
	}
	cuts_num = tmp;
      }
      break;

      case 'n':
      {
	uint64_t tmp = (uint64_t) strtoull (optarg, /* endptr = */ NULL, /* base = */ 10);
	if (tmp <= 0)
	{
	  fprintf (stderr, "Not a valid number of iterations: %s\n", optarg);
	  exit_usage ();
	}
	iterations_num = tmp;
      }
      break;

      case 'h':
      default:
	exit_usage ();
    }
  }

  if ((argc - optind) >= 1)
    input_file = argv[optind];

  return (0);
} /* }}} int read_options */

static int create_random_cut (const sn_network_t *n) /* {{{ */
{
  sn_network_t *n_copy;
  int mask[SN_NETWORK_INPUT_NUM (n)];
  int cuts_left = cuts_num;
  int status;

  memset (mask, 0, sizeof (mask));
  while (cuts_left > 0)
  {
    int input = sn_bounded_random (0, SN_NETWORK_INPUT_NUM (n));

    if (mask[input] != 0)
      continue;

    mask[input] = (2 * sn_bounded_random (0, 1)) - 1;
    cuts_left--;
  }

  n_copy = sn_network_clone (n);
  if (n_copy == NULL)
    return (ENOMEM);

  status = sn_network_cut (n_copy, mask);
  if (status != 0)
  {
    sn_network_destroy (n_copy);
    return (status);
  }

  sn_network_unify (n_copy);

  sn_hashtable_account (hashtable, n_copy);

  sn_network_destroy (n_copy);
  return (0);
} /* }}} int create_random_cut */

static int print_stats (sn_network_t *n) /* {{{ */
{
  static _Bool first_call = 1;

  if (first_call)
  {
    printf ("# iterations unique collisions estimate\n");
    first_call = 0;
  }

  printf ("%"PRIu64" %"PRIu64" %"PRIu64" %.0f\n",
      sn_hashtable_get_total (hashtable),
      sn_hashtable_get_networks (hashtable),
      sn_hashtable_get_collisions (hashtable),
      estimate_reachable_networks (n));

  return (0);
} /* }}} int print_stats */

int main (int argc, char **argv) /* {{{ */
{
  sn_network_t *n;
  uint64_t i;

  read_options (argc, argv);

  if ((input_file == NULL)
      || (strcmp ("-", input_file) == 0))
    n = sn_network_read (stdin);
  else
    n = sn_network_read_file (input_file);
  if (n == NULL)
  {
    fprintf (stderr, "Unable to read network.\n");
    exit (EXIT_FAILURE);
  }

  if (cuts_num <= 0)
    cuts_num = SN_NETWORK_INPUT_NUM (n) / 2;

  hashtable = sn_hashtable_create ();
  if (hashtable == NULL)
    exit (EXIT_FAILURE);

  for (i = 0; i < iterations_num; i++)
  {
    create_random_cut (n);

    if (exit_after_collision)
      if (sn_hashtable_get_collisions (hashtable) > 0)
	break;

    if ((i < 100)
	|| ((i < 1000) && (((i + 1) % 10) == 0))
	|| ((i < 10000) && (((i + 1) % 100) == 0))
	|| ((i < 100000) && (((i + 1) % 1000) == 0))
	|| ((i < 1000000) && (((i + 1) % 10000) == 0))
	|| ((i < 10000000) && (((i + 1) % 100000) == 0)))
      print_stats (n);
  }

  if (((i - 1) < 100)
      || (((i - 1) < 1000) && ((i % 10) == 0))
      || (((i - 1) < 10000) && ((i % 100) == 0))
      || (((i - 1) < 100000) && ((i % 1000) == 0))
      || (((i - 1) < 1000000) && ((i % 10000) == 0))
      || (((i - 1) < 10000000) && ((i % 100000) == 0)))
  {
    /* do nothing */
  }
  else
  {
    print_stats (n);
  }

  return (0);
} /* }}} int main */

/* vim: set shiftwidth=2 softtabstop=2 fdm=marker : */
