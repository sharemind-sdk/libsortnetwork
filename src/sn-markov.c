/**
 * libsortnetwork - src/sn-markov.c
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
#include <inttypes.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include "sn_network.h"
#include "sn_random.h"

#if !defined(__GNUC__) || !__GNUC__
# define __attribute__(x) /**/
#endif

static int inputs_num = 16;

static char *initial_input_file = NULL;
static char *best_output_file = NULL;

static sn_network_t *best_solution = NULL;
static int best_rating = INT_MAX;

static _Bool do_loop = 1;
static uint64_t max_iterations = 0;

static void sigint_handler (int signal __attribute__((unused)))
{
  do_loop = 0;
} /* void sigint_handler */

static void exit_usage (const char *name, int status)
{
  printf ("Usage: %s [options]\n"
      "\n"
      "Valid options are:\n"
      "  -i <file>     Initial input file (REQUIRED)\n"
      "  -o <file>     Write the current best solution to <file>\n"
      "  -n <number>   Maximum number of steps (iterations) to perform\n"
      "  -h            Display usage information (this message)\n"
      "\n",
      name);
  exit (status);
} /* void exit_usage */

int read_options (int argc, char **argv)
{
  int option;

  while ((option = getopt (argc, argv, "i:o:n:h")) != -1)
  {
    switch (option)
    {
      case 'i':
      {
	if (initial_input_file != NULL)
	  free (initial_input_file);
	initial_input_file = strdup (optarg);
	break;
      }

      case 'o':
      {
	if (best_output_file != NULL)
	  free (best_output_file);
	best_output_file = strdup (optarg);
	break;
      }

      case 'n':
      {
	errno = 0;
	max_iterations = (uint64_t) strtoull (optarg,
	    /* endptr = */ NULL,
	    /* base   = */ 0);
	if (errno != 0)
	{
	  fprintf (stderr, "Parsing integer argument failed: %s\n",
	      strerror (errno));
	  exit_usage (argv[0], EXIT_FAILURE);
	}
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0], EXIT_SUCCESS);
    }
  }

  return (0);
} /* int read_options */

static int rate_network (const sn_network_t *n)
{
  int rate;

  rate = SN_NETWORK_STAGE_NUM (n) * SN_NETWORK_INPUT_NUM (n);
  rate += sn_network_get_comparator_num (n);

  return (rate);
} /* int rate_network */

static sn_network_t *get_next (sn_network_t *n)
{
  sn_network_t *nleft;
  sn_network_t *nright;
  sn_network_t *nret;

  if (n == NULL)
    return (NULL);

  nleft = sn_network_clone (n);
  nright = sn_network_clone (n);

  assert (nleft != NULL);
  assert (nright != NULL);

  /* combine the two parents */
  nret = sn_network_combine (nleft, nright);

  sn_network_destroy (nleft);
  sn_network_destroy (nright);

  /* randomly chose an input and do a min- or max-cut. */
  while (SN_NETWORK_INPUT_NUM (nret) > inputs_num)
  {
    int pos;
    enum sn_network_cut_dir_e dir;

    pos = sn_bounded_random (0, SN_NETWORK_INPUT_NUM (nret) - 1);
    dir = (sn_bounded_random (0, 1) == 0) ? DIR_MIN : DIR_MAX;

    assert ((pos >= 0) && (pos < SN_NETWORK_INPUT_NUM (nret)));

    sn_network_cut_at (nret, pos, dir);
  }

  /* compress the network to get a compact representation */
  sn_network_compress (nret);

  assert (SN_NETWORK_INPUT_NUM (nret) == inputs_num);
  return (nret);
} /* sn_network_t *get_next */

static void random_walk (sn_network_t *n)
{
  uint64_t iteration_counter = 0;

  while (do_loop)
  {
    sn_network_t *next = get_next (n);
    int rating;

    if (next == NULL)
    {
      fprintf (stderr, "get_next() failed.\n");
      return;
    }

    rating = rate_network (next);
    if (rating < best_rating)
    {
      printf ("New best rating (%i) after %"PRIu64" iterations.\n",
	  rating, iteration_counter);

      best_rating = rating;
      sn_network_destroy (best_solution);
      best_solution = sn_network_clone (next);
    }

    sn_network_destroy (n);
    n = next;
    iteration_counter++;

    if ((max_iterations > 0) && (iteration_counter >= max_iterations))
      break;
  } /* while (do_loop) */

  sn_network_destroy (n);

  printf ("Exiting after %"PRIu64" iterations.\n", iteration_counter);
} /* void random_walk */

int main (int argc, char **argv)
{
  sn_network_t *start_network;

  struct sigaction sigint_action;
  struct sigaction sigterm_action;

  read_options (argc, argv);
  if (initial_input_file == NULL)
    exit_usage (argv[0], EXIT_FAILURE);

  memset (&sigint_action, '\0', sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  memset (&sigterm_action, '\0', sizeof (sigterm_action));
  sigterm_action.sa_handler = sigint_handler;
  sigaction (SIGTERM, &sigterm_action, NULL);

  start_network = sn_network_read_file (initial_input_file);
  if (start_network == NULL)
  {
    printf ("start_network == NULL\n");
    exit (EXIT_FAILURE);
  }

  inputs_num = SN_NETWORK_INPUT_NUM(start_network);

  printf ("Current configuration:\n"
      "  Initial network:  %s\n"
      "  Number of inputs: %3i\n"
      "=======================\n",
      initial_input_file, inputs_num);

  random_walk (start_network);
  start_network = NULL;

  if (best_solution != NULL)
  {
    if (best_output_file != NULL)
      sn_network_write_file (best_solution, best_output_file);

    printf ("=== Best solution (rating: %i) ===\n", best_rating);
    sn_network_normalize (best_solution);
    sn_network_show (best_solution);
    sn_network_destroy (best_solution);
  }

  exit (EXIT_SUCCESS);
  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
