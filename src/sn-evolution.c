/**
 * libsortnetwork - src/sn-evolution.c
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
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>

#include <pthread.h>

#include <population.h>

#include "sn_network.h"
#include "sn_random.h"

#if !defined(__GNUC__) || !__GNUC__
# define __attribute__(x) /**/
#endif

/* Yes, this is ugly, but the GNU libc doesn't export it with the above flags.
 * */
char *strdup (const char *s);

static uint64_t iteration_counter = 0;
static int inputs_num = 16;
static int inputs_num_is_power_of_two = 1;

static char *initial_input_file = NULL;
static char *best_output_file = NULL;

static int stats_interval = 0;

static int max_population_size = 128;
static population_t *population;

static int evolution_threads_num = 4;

static int do_loop = 0;

static void sigint_handler (int signal __attribute__((unused)))
{
  do_loop++;
} /* void sigint_handler */

static void exit_usage (const char *name)
{
  printf ("Usage: %s [options]\n"
      "\n"
      "Valid options are:\n"
      "  -i <file>     Initial input file (REQUIRED)\n"
      "  -o <file>     Write the current best solution to <file>\n"
      "  -p <num>      Size of the population (default: 128)\n"
      "  -P <peer>     Send individuals to <peer> (may be repeated)\n"
      "  -t <num>      Number of threads (default: 4)\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv)
{
  int option;

  while ((option = getopt (argc, argv, "i:o:p:P:s:t:h")) != -1)
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

      case 'p':
      {
	int tmp = atoi (optarg);
	if (tmp > 0)
	{
	  max_population_size = tmp;
	  population_set_size (population, (size_t) max_population_size);
	}
	break;
      }

      case 'P':
      {
	int status;

	status = population_add_peer (population, optarg, /* port = */ NULL);
	if (status != 0)
	{
	  fprintf (stderr, "population_add_peer failed with status %i.\n",
	      status);
	}
	break;
      }

      case 's':
      {
	int tmp = atoi (optarg);
	if (tmp > 0)
	  stats_interval = tmp;
	break;
      }

      case 't':
      {
	int tmp = atoi (optarg);
	if (tmp >= 1)
	  evolution_threads_num = tmp;
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0]);
    }
  }

  return (0);
} /* int read_options */

static int rate_network (const sn_network_t *n)
{
  int rate;
  int i;

  rate = SN_NETWORK_STAGE_NUM (n) * SN_NETWORK_INPUT_NUM (n);
  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
  {
    sn_stage_t *s = SN_NETWORK_STAGE_GET (n, i);
    rate += SN_STAGE_COMP_NUM (s);
  }

  return (rate);
} /* int rate_network */

static int mutate_network (sn_network_t *n)
{
  sn_network_t *n_copy;
  int stage_index;
  sn_stage_t *s;
  int comparator_index;
  int status;

  n_copy = sn_network_clone (n);
  if (n_copy == NULL)
  {
    fprintf (stderr, "mutate_network: sn_network_clone failed.\n");
    return (-1);
  }

  stage_index = sn_bounded_random (0, SN_NETWORK_STAGE_NUM (n_copy) - 1);
  s = SN_NETWORK_STAGE_GET (n_copy, stage_index);

  comparator_index = sn_bounded_random (0, SN_STAGE_COMP_NUM (s) - 1);
  sn_stage_comparator_remove (s, comparator_index);

  status = sn_network_brute_force_check (n_copy);
  
  sn_network_destroy (n_copy);

  if (status < 0)
    return (-1);
  else if (status > 0) /* Mutated network does not sort anymore. */
    return (1);

  /* We saved one comparator \o/ Let's do the same change on the original
   * network. */
  s = SN_NETWORK_STAGE_GET (n, stage_index);
  sn_stage_comparator_remove (s, comparator_index);

  return (0);
} /* int mutate_network */

static int create_offspring (void)
{
  sn_network_t *p0;
  sn_network_t *p1;
  sn_network_t *n;

  p0 = population_get_random (population);
  p1 = population_get_random (population);

  assert (p0 != NULL);
  assert (p1 != NULL);

  /* combine the two parents */
  n = sn_network_combine (p0, p1, inputs_num_is_power_of_two);

  sn_network_destroy (p0);
  sn_network_destroy (p1);

  /* randomly chose an input and do a min- or max-cut. */
  while (SN_NETWORK_INPUT_NUM (n) > inputs_num)
  {
    int pos;
    enum sn_network_cut_dir_e dir;

    pos = sn_bounded_random (0, SN_NETWORK_INPUT_NUM (n) - 1);
    dir = (sn_bounded_random (0, 1) == 0) ? DIR_MIN : DIR_MAX;

    assert ((pos >= 0) && (pos < SN_NETWORK_INPUT_NUM (n)));

    sn_network_cut_at (n, pos, dir);
  }

  /* compress the network to get a compact representation */
  sn_network_compress (n);

  assert (SN_NETWORK_INPUT_NUM (n) == inputs_num);

  if ((SN_NETWORK_INPUT_NUM (n) <= 16) && (sn_bounded_random (0, 100) <= 1))
    mutate_network (n);

  population_insert (population, n);

  sn_network_destroy (n);

  return (0);
} /* int create_offspring */

static void *evolution_thread (void *arg __attribute__((unused)))
{
  while (do_loop == 0)
  {
    create_offspring ();
    /* XXX: Not synchronized! */
    iteration_counter++;
  }

  return ((void *) 0);
} /* int start_evolution */

static int evolution_start (int threads_num)
{
  pthread_t threads[threads_num]; /* C99 ftw! */
  int i;

  for (i = 0; i < threads_num; i++)
  {
    int status;

    status = pthread_create (&threads[i], /* attr = */ NULL,
	evolution_thread, /* arg = */ NULL);
    if (status != 0)
    {
      fprintf (stderr, "evolution_start: pthread_create[%i] failed "
	  "with status %i.\n",
	  i, status);
      threads[i] = 0;
    }
  }

  while (do_loop == 0)
  {
    int status;
    
    status = sleep (1);
    if (status == 0)
    {
      sn_network_t *n;
      int stages_num;
      int comparators_num;
      int rating;
      int iter;

      iter = iteration_counter;

      n = population_get_fittest (population);

      rating = rate_network (n);

      stages_num = SN_NETWORK_STAGE_NUM (n);
      comparators_num = 0;
      for (i = 0; i < stages_num; i++)
      {
	sn_stage_t *s;

	s = SN_NETWORK_STAGE_GET (n, i);
	comparators_num += SN_STAGE_COMP_NUM (s);
      }

      sn_network_destroy (n);

      printf ("Best after approximately %i iterations: "
	  "%i comparators in %i stages. Rating: %i.\n",
	  iter, comparators_num, stages_num, rating);
    }
  }

  for (i = 0; i < threads_num; i++)
  {
    if (threads[i] == 0)
      continue;
    pthread_join (threads[i], /* value_ptr = */ NULL);
  }

  return (0);
} /* int evolution_start */

int main (int argc, char **argv)
{
  struct sigaction sigint_action;
  struct sigaction sigterm_action;

  population = population_create ((pi_rate_f) rate_network,
      (pi_copy_f) sn_network_clone,
      (pi_free_f) sn_network_destroy);
  if (population == NULL)
  {
    fprintf (stderr, "population_create failed.\n");
    return (1);
  }

  population_set_serialization (population,
      (pi_serialize_f) sn_network_serialize,
      (pi_unserialize_f) sn_network_unserialize);

  read_options (argc, argv);
  if (initial_input_file == NULL)
    exit_usage (argv[0]);

  memset (&sigint_action, '\0', sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  memset (&sigterm_action, '\0', sizeof (sigterm_action));
  sigterm_action.sa_handler = sigint_handler;
  sigaction (SIGTERM, &sigterm_action, NULL);

  population_start_listen_thread (population, NULL, NULL);

  {
    sn_network_t *n;
    int tmp;

    n = sn_network_read_file (initial_input_file);
    if (n == NULL)
    {
      printf ("n == NULL\n");
      return (1);
    }

    inputs_num = SN_NETWORK_INPUT_NUM(n);

    /* Determine if `inputs_num' is a power of two. If so, more merge
     * algorithms can be used. */
    tmp = inputs_num;
    inputs_num_is_power_of_two = 1;
    while (tmp > 0)
    {
      if ((tmp % 2) != 0)
      {
	if (tmp == 1)
	  inputs_num_is_power_of_two = 1;
	else
	  inputs_num_is_power_of_two = 0;
	break;
      }
      tmp = tmp >> 1;
    }

    population_insert (population, n);
    sn_network_destroy (n);
  }

  printf ("Current configuration:\n"
      "  Initial network:  %s\n"
      "  Number of inputs: %3i\n"
      "  Population size:  %3i\n"
      "=======================\n",
      initial_input_file, inputs_num, max_population_size);

  evolution_start (evolution_threads_num);

  printf ("Exiting after %llu iterations.\n",
      (unsigned long long) iteration_counter);

  {
    sn_network_t *n;

    n = population_get_fittest (population);
    if (n != NULL)
    {
      if (best_output_file != NULL)
	sn_network_write_file (n, best_output_file);
      sn_network_show (n);
      sn_network_destroy (n);
    }
  }

  population_destroy (population);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
