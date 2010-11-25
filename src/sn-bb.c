/**
 * libsortnetwork - src/sn-bb-merge.c
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
# define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

#include <math.h>

#include <pthread.h>

#include "sn_network.h"
#include "sn_random.h"

#if !defined(__GNUC__) || !__GNUC__
# define __attribute__(x) /**/
#endif

#if BUILD_MERGE
static int inputs_left = 0;
static int inputs_right = 0;
#else
static int inputs_num = 0;
#endif
static int comparators_num = -1;
static int max_depth = -1;
static int max_stages = INT_MAX;

static char *initial_input_file = NULL;

static void exit_usage (const char *name) /* {{{ */
{
  printf ("Usage: %s [options]\n"
      "\n"
      "Valid options are:\n"
#if BUILD_MERGE
      "  -i <inputs>[:<inputs>]    Number of inputs (left and right)\n"
#else
      "  -i <inputs>               Number of inputs\n"
#endif
      "  -c <comparators>          Number of comparators\n"
      "  -s <stages>               Maximum number of stages\n"
      "  -I <file>                 Initial input file\n"
      "\n",
      name);
  exit (1);
} /* }}} void exit_usage */

int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "i:c:I:o:p:P:s:t:h")) != -1)
  {
    switch (option)
    {
#if BUILD_MERGE
      case 'i':
      {
	int tmp_left;
	int tmp_right;

        char *tmp_str;

        tmp_str = strchr (optarg, ':');
        if (tmp_str != NULL)
        {
          *tmp_str = 0;
          tmp_str++;
          tmp_right = atoi (tmp_str);
        }
        else
        {
          tmp_right = 0;
        }

        tmp_left = atoi (optarg);

        if (tmp_left <= 0)
          exit_usage (argv[0]);

        if (tmp_right <= 0)
          tmp_right = tmp_left;

        inputs_left = tmp_left;
        inputs_right = tmp_right;

	break;
      }
#else
      case 'i':
      {
	int tmp;
	tmp = atoi (optarg);
	if (tmp > 0)
          inputs_num = tmp;
	break;
      }
#endif

      case 'c':
      {
	int tmp;
	tmp = atoi (optarg);
	if (tmp > 0)
	  comparators_num = tmp;
	break;
      }

      case 'I':
      {
	if (initial_input_file != NULL)
	  free (initial_input_file);
	initial_input_file = strdup (optarg);
	break;
      }

      case 's':
      {
	int tmp;
	tmp = atoi (optarg);
	if (tmp > 0)
	  max_stages = tmp;
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0]);
    }
  }

  return (0);
} /* }}} int read_options */

#if BUILD_MERGE
static int rate_network (sn_network_t *n) /* {{{ */
{
  int test_pattern[n->inputs_num];
  int values[n->inputs_num];

  int patterns_failed = 0;

  int zeros_left;
  int zeros_right;

  int i;

  assert (n->inputs_num == (inputs_left + inputs_right));

  memset (test_pattern, 0, sizeof (test_pattern));
  for (i = 0; i < inputs_left; i++)
    test_pattern[i] = 1;

  for (zeros_left = 0; zeros_left <= inputs_left; zeros_left++)
  {
    int status;
    int previous;

    if (zeros_left > 0)
      test_pattern[zeros_left - 1] = 0;

    for (i = 0; i < inputs_right; i++)
      test_pattern[inputs_left + i] = 1;

    for (zeros_right = 0; zeros_right <= inputs_right; zeros_right++)
    {
      if (zeros_right > 0)
        test_pattern[inputs_left + zeros_right - 1] = 0;

      /* Copy the current pattern and let the network sort it */
      memcpy (values, test_pattern, sizeof (values));
      status = sn_network_sort (n, values);
      if (status != 0)
        return (status);

      /* Check if the array is now sorted. */
      previous = values[0];
      status = 0;
      for (i = 1; i < n->inputs_num; i++)
      {
        if (previous > values[i])
        {
          patterns_failed++;
          status = -1;
          break;
        }
        previous = values[i];
      }
    } /* for (zeros_right) */
  } /* for (zeros_left) */

  return (patterns_failed);
} /* }}} int rate_network */
#else
static int rate_network (sn_network_t *n) /* {{{ */
{
  int test_pattern[n->inputs_num];
  int values[n->inputs_num];

  int patterns_sorted = 0;
  int patterns_failed = 0;

  memset (test_pattern, 0, sizeof (test_pattern));
  while (42)
  {
    int previous;
    int overflow;
    int status;
    int i;

    /* Copy the current pattern and let the network sort it */
    memcpy (values, test_pattern, sizeof (values));
    status = sn_network_sort (n, values);
    if (status != 0)
      return (status);

    /* Check if the array is now sorted. */
    previous = values[0];
    status = 0;
    for (i = 1; i < n->inputs_num; i++)
    {
      if (previous > values[i])
      {
	patterns_failed++;
	status = -1;
	break;
      }
      previous = values[i];
    }

    if (status == 0)
      patterns_sorted++;

    /* Generate the next test pattern */
    overflow = 1;
    for (i = 0; i < n->inputs_num; i++)
    {
      if (test_pattern[i] == 0)
      {
        test_pattern[i] = 1;
        overflow = 0;
        break;
      }
      else
      {
        test_pattern[i] = 0;
        overflow = 1;
      }
    }

    /* Break out of the while loop if we tested all possible patterns */
    if (overflow == 1)
      break;
  } /* while (42) */

  /* All tests successfull */
  return (patterns_failed);
} /* }}} int rate_network */
#endif

static _Bool sn_bound (sn_network_t *n, int depth, int rating) /* {{{ */
{
  static int least_failed = INT_MAX;

  int lower_bound;

  assert (depth <= max_depth);

  if (SN_NETWORK_STAGE_NUM (n) > max_stages)
    return (1);

  /* Minimum number of comparisons requires */
  lower_bound = (int) (ceil (log ((double) rating) / log (2.0)));

  if (lower_bound > (max_depth - depth))
    return (1);

  if (least_failed >= rating)
  {
    printf ("New optimum: %i\n", rating);
    sn_network_show (n);
    printf ("===\n\n");
    fflush (stdout);
    least_failed = rating;

    /* FIXME */
    if (rating == 0)
      exit (EXIT_SUCCESS);
  }

  return (0);
} /* }}} _Bool sn_bound */

static int sn_branch (sn_network_t *n, int depth, int rating) /* {{{ */
{
  int left_num;
  int left_rnd;
  int i;

  if (depth >= max_depth)
    return (-1);

  if (rating < 0)
    rating = rate_network (n);

  left_num = SN_NETWORK_INPUT_NUM (n) - 1;
  left_rnd = sn_bounded_random (0, left_num - 1);

  for (i = 0; i < left_num; i++)
  {
    int left_input;
    int right_num;
    int right_rnd;
    int j;

    left_input = left_rnd + i;
    if (left_input >= left_num)
      left_input -= left_num;
    assert (left_input < left_num);
    assert (left_input >= 0);

    right_num = SN_NETWORK_INPUT_NUM (n) - (left_input + 1);
    if (right_num <= 1)
      right_rnd = 0;
    else
      right_rnd = sn_bounded_random (0, right_num - 1);

    for (j = 0; j < right_num; j++)
    {
      sn_network_t *n_copy;
      sn_comparator_t *c;
      int n_copy_rating;
      int right_input;

      right_input = (left_input + 1) + right_rnd + j;
      if (right_input >= SN_NETWORK_INPUT_NUM (n))
        right_input -= right_num;
      assert (right_input < SN_NETWORK_INPUT_NUM (n));
      assert (right_input > left_input);

      n_copy = sn_network_clone (n);
      c = sn_comparator_create (left_input, right_input);

      sn_network_comparator_add (n_copy, c);

      /* Make sure the new comparator is improving the network */
      n_copy_rating = rate_network (n_copy);
      if (n_copy_rating >= rating)
      {
        sn_comparator_destroy (c);
        sn_network_destroy (n_copy);
        continue;
      }

      if (!sn_bound (n_copy, depth + 1, n_copy_rating))
        sn_branch (n_copy, depth + 1, n_copy_rating);

      sn_comparator_destroy (c);
      sn_network_destroy (n_copy);
    } /* for (right_input) */
  } /* for (left_input) */

  return (0);
} /* }}} int sn_branch */

int main (int argc, char **argv) /* {{{ */
{
  sn_network_t *n;
  int c_num;
#if BUILD_MERGE
  int inputs_num;
#endif

  read_options (argc, argv);

#if BUILD_MERGE
  if ((inputs_left <= 0) || (inputs_right <= 0) || (comparators_num <= 0))
    exit_usage (argv[0]);
  inputs_num = inputs_left + inputs_right;
#else
  if ((inputs_num <= 0) || (comparators_num <= 0))
    exit_usage (argv[0]);
#endif

  if (initial_input_file != NULL)
  {
    n = sn_network_read_file (initial_input_file);
    if (n == NULL)
    {
      fprintf (stderr, "Cannot read network from `%s'.\n",
	  initial_input_file);
      exit (EXIT_FAILURE);
    }

    if (n->inputs_num != inputs_num)
    {
      fprintf (stderr, "Network `%s' has %i inputs, but %i were configured "
	  "on the command line.\n",
	  initial_input_file, n->inputs_num, inputs_num);

      exit (EXIT_FAILURE);
    }

    if (SN_NETWORK_STAGE_NUM (n) > max_stages)
    {
      fprintf (stderr, "The maximum number of stages allowed (%i) is smaller "
          "than the number of stages of the input file (%i).\n",
          max_stages, SN_NETWORK_STAGE_NUM (n));
      exit (EXIT_FAILURE);
    }
  }
  else /* if (initial_input_file == NULL) */
  {
    n = sn_network_create (inputs_num);
  }

  c_num = sn_network_get_comparator_num (n);
  if (c_num >= comparators_num)
  {
    fprintf (stderr, "The initial network has already %i comparators, "
        "which conflicts with the -c option (%i)!\n",
        c_num, comparators_num);
    exit (EXIT_FAILURE);
  }
  max_depth = comparators_num - c_num;

  printf ("Current configuration:\n"
      "  Number of inputs:      %3i\n"
      "  Number of comparators: %3i\n"
      "  Search depth:          %3i\n"
      "=======================\n",
      inputs_num, comparators_num, max_depth);

  sn_branch (n, /* depth = */ 0, /* rating = */ -1);

  return (0);
} /* }}} int main */

/* vim: set sw=2 sts=2 et fdm=marker : */
