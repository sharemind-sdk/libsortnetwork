/**
 * libsortnetwork - src/sn-evolution-cut.c
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
#include <stdint.h>
#include <inttypes.h>
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

typedef int individuum_t;

static int inputs_num = 0;
static int outputs_num = 0;
static int cuts_num = 0;

static char *initial_input_file = NULL;
static const sn_network_t *initial_network = NULL;
static char *best_output_file = NULL;

static int stats_interval = 0;

static int max_population_size = 128;
static population_t *population = NULL;

static int evolution_threads_num = 4;

static int do_loop = 0;
static uint64_t iteration_counter = 0;

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
      "  -O <num>      Number of outputs (default: inputs / 2)\n"
      "  -p <num>      Size of the population (default: 128)\n"
      "  -P <peer>     Send individuals to <peer> (may be repeated)\n"
      "  -t <num>      Number of threads (default: 4)\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "i:o:O:p:P:s:t:h")) != -1)
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

      case 'O':
      {
	int tmp = atoi (optarg);
	if (tmp > 0)
	  outputs_num = tmp;
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
} /* }}} int read_options */

static int apply_cut (sn_network_t *n, int input) /* {{{ */
{
  enum sn_network_cut_dir_e dir = DIR_MAX;

  if (input < 0)
  {
    dir = DIR_MIN;
    input *= (-1);
  }
  input--;

  return (sn_network_cut_at (n, input, dir));
} /* }}} int apply_cut */

static int apply (sn_network_t *n, const individuum_t *ind) /* {{{ */
{
  int i;

  for (i = 0; i < cuts_num; i++)
    apply_cut (n, ind[i]);

  return (0);
} /* }}} int apply */

static int rate_network (const sn_network_t *n) /* {{{ */
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
} /* }}} int rate_network */

static sn_network_t *individuum_to_network (const individuum_t *ind) /* {{{ */
{
  sn_network_t *n;

  n = sn_network_clone (initial_network);
  if (n == NULL)
    return (NULL);

  apply (n, ind);

  sn_network_normalize (n);
  sn_network_compress (n);

  return (n);
} /* }}} sn_network_t *individuum_to_network */

static int ind_rate (const void *arg) /* {{{ */
{
  sn_network_t *n;
  int status;

  n = individuum_to_network (arg);
  if (n == NULL)
    return (-1);

  status = rate_network (n);
  sn_network_destroy (n);

  return (status);
} /* }}} int ind_rate */

static void *ind_copy (const void *in) /* {{{ */
{
  size_t s = sizeof (individuum_t) * cuts_num;
  void *out;

  out = malloc (s);
  if (out == NULL)
    return (NULL);

  memcpy (out, in, s);
  return (out);
} /* }}} void *ind_copy */

static void ind_free (void *ind) /* {{{ */
{
  if (ind != NULL)
    free (ind);
} /* }}} void ind_free */

static void ind_print (const individuum_t *ind)
{
  int i;

  for (i = 0; i < cuts_num; i++)
  {
    int input = ind[i];
    int dir = 0;

    if (input < 0)
    {
      input *= -1;
      dir = 1;
    }
    input--;

    printf ("%s(%3i)\n", (dir == 0) ? "MAX" : "MIN", input);
  }
} /* }}} void ind_print */

static individuum_t *recombine (individuum_t *i0, individuum_t *i1) /* {{{ */
{
  individuum_t *offspring;
  int cut_at;
  int i;

  if ((i0 == NULL) || (i1 == NULL))
    return (NULL);

  offspring = malloc (sizeof (*offspring) * cuts_num);
  if (offspring == NULL)
    return (NULL);
  memset (offspring, 0, sizeof (*offspring) * cuts_num);

  cut_at = sn_bounded_random (0, cuts_num);
  for (i = 0; i < cuts_num; i++)
  {
    if (i < cut_at)
      offspring[i] = i0[i];
    else
      offspring[i] = i1[i];
  }

  return (offspring);
} /* }}} individuum_t *recombine */

static void random_cut (individuum_t *ind, int index) /* {{{ */
{
  int max_input = inputs_num - index;

  ind[index] = 0;
  while (ind[index] == 0)
    ind[index] = sn_bounded_random ((-1) * max_input, max_input);
} /* }}} void random_cut */

static void mutate (individuum_t *ind) /* {{{ */
{
  int reset_input;
  int i;

  reset_input = sn_bounded_random (0, 3 * cuts_num);
  for (i = 0; i < cuts_num; i++)
  {
    if (reset_input == i)
      random_cut (ind, i);

    if (sn_bounded_random (0, 100))
      ind[i] *= (-1);
  }
} /* }}} void mutate */

static int create_offspring (void) /* {{{ */
{
  individuum_t *i0;
  individuum_t *i1;
  individuum_t *i2;

  i0 = population_get_random (population);
  i1 = population_get_random (population);

  i2 = recombine (i0, i1);
  mutate (i2);

  population_insert (population, i2);

  free (i0);
  free (i1);
  free (i2);

  return (0);
} /* }}} int create_offspring */

static void *evolution_thread (void *arg __attribute__((unused))) /* {{{ */
{
  while (do_loop == 0)
  {
    create_offspring ();
    /* XXX: Not synchronized! */
    iteration_counter++;
  }

  return ((void *) 0);
} /* }}} int start_evolution */

static int evolution_start (int threads_num) /* {{{ */
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
      individuum_t *ind;
      sn_network_t *n;
      int stages_num;
      int comparators_num;
      int rating;
      int iter;

      iter = iteration_counter;

      ind = population_get_fittest (population);
      n = individuum_to_network (ind);

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
      ind_free (ind);

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
} /* }}} int evolution_start */

int main (int argc, char **argv) /* {{{ */
{
  struct sigaction sigint_action;
  struct sigaction sigterm_action;
  int i;

  population = population_create (ind_rate, ind_copy, ind_free);
  if (population == NULL)
  {
    fprintf (stderr, "population_create failed.\n");
    return (1);
  }

#if 0
  population_set_serialization (population,
      (pi_serialize_f) sn_network_serialize,
      (pi_unserialize_f) sn_network_unserialize);
#endif

  read_options (argc, argv);
  if (initial_input_file == NULL)
    exit_usage (argv[0]);

  memset (&sigint_action, '\0', sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  memset (&sigterm_action, '\0', sizeof (sigterm_action));
  sigterm_action.sa_handler = sigint_handler;
  sigaction (SIGTERM, &sigterm_action, NULL);

  /* population_start_listen_thread (population, NULL, NULL); */

  initial_network = sn_network_read_file (initial_input_file);
  if (initial_network == NULL)
  {
    fprintf (stderr, "Reading input file failed.\n");
    exit (EXIT_FAILURE);
  }

  inputs_num = SN_NETWORK_INPUT_NUM(initial_network);
  if (outputs_num == 0)
    outputs_num = inputs_num / 2;

  if (outputs_num >= inputs_num)
  {
    fprintf (stderr, "Number of inputs (%i) is smaller than "
        "(or equal to) the number of outputs (%i).\n",
        inputs_num, outputs_num);
    exit (EXIT_FAILURE);
  }

  cuts_num = inputs_num - outputs_num;
  assert (cuts_num >= 1);

  for (i = 0; i < max_population_size; i++)
  { /* create a random initial individuum */
    individuum_t *ind;
    int i;

    ind = malloc (sizeof (*ind) * cuts_num);
    if (ind == NULL)
    {
      fprintf (stderr, "malloc failed.\n");
      exit (EXIT_FAILURE);
    }

    for (i = 0; i < cuts_num; i++)
      random_cut (ind, i);

    population_insert (population, ind);
    ind_free (ind);
  }

  printf ("Current configuration:\n"
      "  Initial network:   %s\n"
      "  Number of inputs:  %3i\n"
      "  Number of outputs: %3i\n"
      "  Population size:   %3i\n"
      "=======================\n",
      initial_input_file, inputs_num, outputs_num, max_population_size);

  evolution_start (evolution_threads_num);

  printf ("Exiting after %"PRIu64" iterations.\n",
      iteration_counter);

  {
    individuum_t *ind;
    sn_network_t *n;

    ind = population_get_fittest (population);
    if (ind != NULL)
    {
      n = individuum_to_network (ind);
      if (best_output_file != NULL)
	sn_network_write_file (n, best_output_file);
      sn_network_show (n);
      sn_network_destroy (n);

      ind_print (ind);
      free (ind);
    }
  }

  population_destroy (population);

  return (0);
} /* }}} int main */

/* vim: set shiftwidth=2 softtabstop=2 et fdm=marker : */
