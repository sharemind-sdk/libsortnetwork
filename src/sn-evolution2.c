/**
 * collectd - src/sn-evolution.c
 * Copyright (C) 2008,2009  Florian octo Forster
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

#define SNE_MIN(a,b) ((a) < (b) ? (a) : (b))
#define SNE_MAX(a,b) ((a) > (b) ? (a) : (b))

/* Yes, this is ugly, but the GNU libc doesn't export it with the above flags.
 * */
char *strdup (const char *s);

static uint64_t iteration_counter = 0;
static int inputs_num = -1;

static int comparators_num = -1;

static char *best_output_file = NULL;

static int stats_interval = 0;

static int max_population_size = 128;
static population_t *population;

static int evolution_threads_num = 4;

static int do_loop = 0;

static void sigint_handler (int signal)
{
  do_loop++;
} /* void sigint_handler */

static void exit_usage (const char *name)
{
  printf ("Usage: %s [options]\n"
      "\n"
      "Valid options are:\n"
      "  -i <inputs>       Number of inputs\n"
      "  -c <comparators>  Number of comparators\n"
      "  -o <file>         Write the current best solution to <file>\n"
      "  -p <num>          Size of the population (default: 128)\n"
      "  -P <peer>         Send individuals to <peer> (may be repeated)\n"
      "  -t <num>          Number of threads (default: 4)\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv)
{
  int option;

  while ((option = getopt (argc, argv, "i:c:o:p:P:s:t:h")) != -1)
  {
    switch (option)
    {
      case 'i':
      {
	int tmp;
	tmp = atoi (optarg);
	if (tmp > 0)
	  inputs_num = tmp;
	break;
      }

      case 'c':
      {
	int tmp;
	tmp = atoi (optarg);
	if (tmp > 0)
	  comparators_num = tmp;
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
  return (SN_NETWORK_STAGE_NUM (n) + patterns_failed);
} /* }}} int rate_network */

static sn_comparator_t get_random_comparator (void) /* {{{ */
{
  sn_comparator_t c;

  c.min = sn_bounded_random (0, inputs_num - 1);
  c.max = c.min;
  while (c.max == c.min)
    c.max = sn_bounded_random (0, inputs_num - 1);

  return (c);
} /* }}} sn_comparator_t get_random_comparator */

static int mutate_network (sn_comparator_t *comparators, int comparators_num)
{
  int i;

  for (i = 0; i < comparators_num; i++)
    if (sn_bounded_random (0, 1000000) >= 972655)
      comparators[i] = get_random_comparator ();

  return (0);
} /* int mutate_network */

static int create_offspring (void)
{
  sn_network_t *p0;
  sn_comparator_t p0_comparators[comparators_num];
  int p0_comparators_num;

  sn_network_t *p1;
  sn_comparator_t p1_comparators[comparators_num];
  int p1_comparators_num;

  sn_network_t *n;
  sn_comparator_t n_comparators[comparators_num];
  int n_comparators_num;

  int i;

  p0 = population_get_random (population);
  p1 = population_get_random (population);

  assert (p0 != NULL);
  assert (p1 != NULL);

  p0_comparators_num = 0;
  for (i = 0; i < SN_NETWORK_STAGE_NUM (p0); i++)
  {
    sn_stage_t *s;
    int j;

    s = SN_NETWORK_STAGE_GET (p0, i);

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c;

      c = SN_STAGE_COMP_GET (s, j);
      p0_comparators[p0_comparators_num] = *c;
      p0_comparators_num++;
    }
  }
  while (p0_comparators_num < comparators_num)
  {
    p0_comparators[p0_comparators_num] = get_random_comparator ();
    p0_comparators_num++;
  }
  sn_network_destroy (p0);
  p0 = NULL;

  p1_comparators_num = 0;
  for (i = 0; i < SN_NETWORK_STAGE_NUM (p1); i++)
  {
    sn_stage_t *s;
    int j;

    s = SN_NETWORK_STAGE_GET (p1, i);

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c;

      c = SN_STAGE_COMP_GET (s, j);
      p1_comparators[p1_comparators_num] = *c;
      p1_comparators_num++;
    }
  }
  while (p1_comparators_num < comparators_num)
  {
    p1_comparators[p1_comparators_num] = get_random_comparator ();
    p1_comparators_num++;
  }
  sn_network_destroy (p1);
  p1 = NULL;

  n_comparators_num = sn_bounded_random (SNE_MIN (p0_comparators_num, p1_comparators_num),
      SNE_MAX (p0_comparators_num, p1_comparators_num));

  for (i = 0; i < n_comparators_num; i++)
  {
    if (i >= p0_comparators_num)
      n_comparators[i] = p1_comparators[i];
    else if (i >= p1_comparators_num)
      n_comparators[i] = p0_comparators[i];
    else if (sn_bounded_random (0, 1) == 0)
      n_comparators[i] = p1_comparators[i];
    else
      n_comparators[i] = p0_comparators[i];

  }

  mutate_network (n_comparators, n_comparators_num);

  n = sn_network_create (inputs_num);
  for (i = 0; i < n_comparators_num; i++)
    sn_network_comparator_add (n, &n_comparators[i]);

  /* compress the network to get a compact representation */
  sn_network_compress (n);

  assert (SN_NETWORK_INPUT_NUM (n) == inputs_num);

  population_insert (population, n);

  sn_network_destroy (n);

  return (0);
} /* int create_offspring */

static void *evolution_thread (void *arg)
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
	  "%i comparators in %i stages. Rating: %i (%i not sorted).\n",
	  iter, comparators_num, stages_num, rating,
	  rating - stages_num);
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

  if ((inputs_num <= 0) || (comparators_num <= 0))
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
    int i;

    n = sn_network_create (inputs_num);
    for (i = 0; i < comparators_num; i++)
    {
      sn_comparator_t c;

      c = get_random_comparator ();
      sn_network_comparator_add (n, &c);
    }

    population_insert (population, n);
    sn_network_destroy (n);
  }

  printf ("Current configuration:\n"
      "  Number of inputs:      %3i\n"
      "  Number of comparators: %3i\n"
      "  Population size:       %3i\n"
      "=======================\n",
      inputs_num, comparators_num, max_population_size);

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
