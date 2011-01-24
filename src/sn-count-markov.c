/**
 * libsortnetwork - src/sn-count-markov.c
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

#include <glib.h>

#include "sn_network.h"
#include "sn_random.h"

#if !defined(__GNUC__) || !__GNUC__
# define __attribute__(x) /**/
#endif

static int inputs_num = 16;

static char *initial_input_file = NULL;

static GTree *all_networks = NULL;
static uint64_t cycles_num = 0;

static _Bool do_loop = 1;
static _Bool do_output_network = 0;
static uint64_t max_iterations = 0;

static void sigint_handler (__attribute__((unused)) int signal) /* {{{ */
{
  do_loop = 0;
} /* }}} void sigint_handler */

static void sighup_handler (__attribute__((unused)) int signal) /* {{{ */
{
  do_output_network = 1;
} /* }}} void sighup_handler */

static int output_network (sn_network_t *n) /* {{{ */
{
  FILE *fh;

  if (n == NULL)
    return (EINVAL);

  fh = fopen ("/dev/tty", "w");
  if (fh == NULL)
    return (errno);

  sn_network_show_fh (n, fh);

  fclose (fh);
  return (0);
} /* }}} int output_network */

static gint compare_hashval (gconstpointer p0, gconstpointer p1) /* {{{ */
{
  const uint64_t *i0 = (gconstpointer) p0;
  const uint64_t *i1 = (gconstpointer) p1;

  if ((*i0) < (*i1))
    return (-1);
  else if ((*i0) > (*i1))
    return (1);
  else
    return (0);
} /* }}} gint account_network_compare */

static int account_network (const sn_network_t *n, uint64_t iteration) /* {{{ */
{
  uint64_t  key;
  uint64_t *key_ptr;
  uint64_t *value_ptr;

  if (n == NULL)
    return (EINVAL);

  if (iteration == 0)
    printf ("# iteration cyclelength\n");

  key = sn_network_get_hashval (n);
  key_ptr = &key;

  value_ptr = (uint64_t *) g_tree_lookup (all_networks, (gconstpointer) key_ptr);
  if (value_ptr != NULL)
  {
    uint64_t cycle_length = iteration - (*value_ptr);
    printf ("%"PRIu64" %"PRIu64"\n", iteration, cycle_length);

    cycles_num++;
    *value_ptr = iteration;
    return (0);
  }

  key_ptr = malloc (sizeof (*key_ptr));
  if (key_ptr == NULL)
    return (ENOMEM);
  *key_ptr = key;

  value_ptr = malloc (sizeof (*value_ptr));
  if (value_ptr == NULL)
  {
    free (key_ptr);
    return (ENOMEM);
  }
  *value_ptr = iteration;

  g_tree_insert (all_networks, (gpointer) key_ptr, (gpointer) value_ptr);
  return (0);
} /* }}} int account_network */

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

  /* Make sure one network is always represented in the same way. */
  sn_network_unify (nret);

  assert (SN_NETWORK_INPUT_NUM (nret) == inputs_num);
  return (nret);
} /* sn_network_t *get_next */

static void random_walk (sn_network_t *n)
{
  uint64_t iteration_counter = 0;

  while (do_loop)
  {
    sn_network_t *next = get_next (n);

    if (next == NULL)
    {
      fprintf (stderr, "get_next() failed.\n");
      return;
    }

    account_network (next, iteration_counter);

    if (do_output_network)
    {
      output_network (next);
      do_output_network = 0;
    }

    sn_network_destroy (n);
    n = next;
    iteration_counter++;

    if ((max_iterations > 0) && (iteration_counter >= max_iterations))
      break;
  } /* while (do_loop) */

  sn_network_destroy (n);

  printf ("# Exiting after %"PRIu64" iterations.\n", iteration_counter);
} /* void random_walk */

int main (int argc, char **argv)
{
  sn_network_t *start_network;

  struct sigaction sigint_action;
  struct sigaction sigterm_action;
  struct sigaction sighup_action;

  read_options (argc, argv);
  if (initial_input_file == NULL)
    exit_usage (argv[0], EXIT_FAILURE);

  memset (&sigint_action, 0, sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  memset (&sigterm_action, '\0', sizeof (sigterm_action));
  sigterm_action.sa_handler = sigint_handler;
  sigaction (SIGTERM, &sigterm_action, NULL);

  memset (&sighup_action, 0, sizeof (sighup_action));
  sighup_action.sa_handler = sighup_handler;
  sigaction (SIGHUP, &sighup_action, NULL);

  all_networks = g_tree_new (compare_hashval);
  if (all_networks == NULL)
  {
    fprintf (stderr, "g_tree_new() failed.\n");
    exit (EXIT_FAILURE);
  }

  start_network = sn_network_read_file (initial_input_file);
  if (start_network == NULL)
  {
    printf ("start_network == NULL\n");
    exit (EXIT_FAILURE);
  }

  inputs_num = SN_NETWORK_INPUT_NUM(start_network);

  random_walk (start_network);
  start_network = NULL;

  printf ("# %"PRIu64" cycles were detected.\n", cycles_num);

  exit (EXIT_SUCCESS);
  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 fdm=marker : */
