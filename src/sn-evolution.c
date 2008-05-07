/**
 * collectd - src/sn-evolution.c
 * Copyright (C) 2008  Florian octo Forster
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

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

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

#include "sn_network.h"
#include "sn_population.h"
#include "sn_random.h"

/* Yes, this is ugly, but the GNU libc doesn't export it with the above flags.
 * */
char *strdup (const char *s);

static uint64_t iteration_counter = 0;
static int inputs_num = 16;

static char *initial_input_file = NULL;
static char *best_output_file = NULL;

static int stats_interval = 0;

static int max_population_size = 128;
static sn_population_t *population;

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
      "  -i <file>     Initial input file (REQUIRED)\n"
      "  -o <file>     Write the current best solution to <file>\n"
      "  -p <num>      Size of the population (default: 128)\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv)
{
  int option;

  while ((option = getopt (argc, argv, "i:o:p:P:s:h")) != -1)
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
	  max_population_size = tmp;
	break;
      }

      case 's':
      {
	int tmp = atoi (optarg);
	if (tmp > 0)
	  stats_interval = tmp;
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0]);
    }
  }

  return (0);
} /* int read_options */

#if 0
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
#endif

#if 0
static int population_print_stats (int iterations)
{
  int best = -1;
  int total = 0;
  int i;

  for (i = 0; i < population_size; i++)
  {
    if ((best == -1) || (best > population[i].rating))
      best = population[i].rating;
    total += population[i].rating;
  }

  printf ("Iterations: %6i; Best: %i; Average: %.2f;\n",
      iterations, best, ((double) total) / ((double) population_size));

  return (0);
} /* int population_print_stats */
#endif

#if 0
static int insert_into_population (sn_network_t *n)
{
  int rating;
  int worst_rating;
  int worst_index;
  int best_rating;
  int nmemb;
  int i;

  rating = rate_network (n);

  if (population_size < max_population_size)
  {
    population[population_size].network = n;
    population[population_size].rating  = rating;
    population_size++;
    return (0);
  }

  worst_rating = -1;
  worst_index  = -1;
  best_rating  = -1;
  for (i = 0; i < olymp_size; i++)
  {
    if (population[i].rating > worst_rating)
    {
      worst_rating = population[i].rating;
      worst_index  = i;
    }
    if ((population[i].rating < best_rating)
	|| (best_rating == -1))
      best_rating = population[i].rating;
  }

  if (rating < best_rating)
  {
    if (best_output_file != NULL)
    {
      printf ("Writing network with rating %i to %s\n",
	  rating, best_output_file);
      sn_network_write_file (n, best_output_file);
    }
    else
    {
      printf ("New best solution has rating %i\n",
	  rating);
    }
  }

  nmemb = max_population_size - (worst_index + 1);

  sn_network_destroy (population[worst_index].network);
  population[worst_index].network = NULL;

  memmove (population + worst_index,
      population + (worst_index + 1),
      nmemb * sizeof (population_entry_t));

  population[max_population_size - 1].network = n;
  population[max_population_size - 1].rating  = rating;

  return (0);
} /* int insert_into_population */
#endif

static int create_offspring (void)
{
  sn_network_t *p0;
  sn_network_t *p1;
  sn_network_t *n;

  p0 = sn_population_pop (population);
  p1 = sn_population_pop (population);

  assert (p0 != NULL);
  assert (p1 != NULL);

  /* combine the two parents */
  n = sn_network_combine (p0, p1);

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

  sn_population_push (population, n);

  sn_network_destroy (n);

  return (0);
} /* int create_offspring */

static int start_evolution (void)
{
  uint64_t i;

  while (do_loop == 0)
  {
    create_offspring ();

    iteration_counter++;
    i = iteration_counter;

    if ((i % 1000) == 0)
    {
      int rating = sn_population_best_rating (population);
      printf ("After %10llu iterations: Best rating: %4i\n", i, rating);
    }
  }

  return (0);
} /* int start_evolution */

int main (int argc, char **argv)
{
  struct sigaction sigint_action;

  read_options (argc, argv);
  if (initial_input_file == NULL)
    exit_usage (argv[0]);

  memset (&sigint_action, '\0', sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  population = sn_population_create (max_population_size);
  if (population == NULL)
  {
    fprintf (stderr, "sn_population_create failed.\n");
    return (1);
  }

  {
    sn_network_t *n;

    n = sn_network_read_file (initial_input_file);
    if (n == NULL)
    {
      printf ("n == NULL\n");
      return (1);
    }

    inputs_num = SN_NETWORK_INPUT_NUM(n);

    sn_population_push (population, n);
    sn_network_destroy (n);
  }

  printf ("Current configuration:\n"
      "  Initial network:  %s\n"
      "  Number of inputs: %3i\n"
      "  Population size:  %3i\n"
      "=======================\n",
      initial_input_file, inputs_num, max_population_size);

  start_evolution ();

  printf ("Exiting after %llu iterations.\n", iteration_counter);

  {
    sn_network_t *n;

    n = sn_population_best (population);
    if (n != NULL)
    {
      if (best_output_file != NULL)
	sn_network_write_file (n, best_output_file);
      else
	sn_network_show (n);
      sn_network_destroy (n);
    }
  }

  sn_population_destroy (population);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
