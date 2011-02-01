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
#include <errno.h>

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
static uint64_t max_iterations = 0;

static int target_rating = 0;

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
      "  -n <num>      Maximum number of steps (iterations) to perform\n"
      "  -r <num>      Target rating: Stop loop when rating is less then or equal\n"
      "                to this number.\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "i:o:O:p:P:s:t:n:r:h")) != -1)
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
	  exit_usage (argv[0]);
	}
	break;
      }

      case 'r':
      {
	int tmp = atoi (optarg);
	if (tmp > 0)
          target_rating = tmp;
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0]);
    }
  }

  return (0);
} /* }}} int read_options */

static int rate_network (const sn_network_t *n) /* {{{ */
{
  int rate;

  rate = SN_NETWORK_STAGE_NUM (n) * SN_NETWORK_INPUT_NUM (n)
    + sn_network_get_comparator_num (n);

  return (rate);
} /* }}} int rate_network */

static sn_network_t *individuum_to_network (const individuum_t *ind) /* {{{ */
{
  sn_network_t *n;
  int mask[SN_NETWORK_INPUT_NUM (initial_network)];

  memcpy (mask, ind, sizeof (mask));

  n = sn_network_clone (initial_network);
  if (n == NULL)
    return (NULL);

  sn_network_cut (n, mask);

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
  size_t s = sizeof (individuum_t) * inputs_num;
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

static void ind_print (const individuum_t *ind) /* {{{ */
{
  int i;

  for (i = 0; i < inputs_num; i++)
  {
    printf ("%3i: %s\n", i, (ind[i] == 0) ? "-" :
        (ind[i] < 0) ? "MIN" : "MAX");
  }

  printf ("# sn-cut");
  for (i = 0; i < inputs_num; i++)
  {
    if (ind[i] == 0)
      continue;
    printf (" %i:%s", i, (ind[i] < 0) ? "MIN" : "MAX");
  }
  printf ("\n");
} /* }}} void ind_print */

/* Simply makes sure the right amount of cutting positions exist. */
static void mutate (individuum_t *ind, int this_cuts_num) /* {{{ */
{
  int i;

  if (this_cuts_num < 0)
  {
    this_cuts_num = 0;
    for (i = 0; i < inputs_num; i++)
      if (ind[i] != 0)
        this_cuts_num++;
  }

  while (this_cuts_num != cuts_num)
  {
    i = sn_bounded_random (0, inputs_num - 1);

    if ((this_cuts_num < cuts_num)
        && (ind[i] == 0))
    {
      ind[i] = (sn_bounded_random (0, 1) * 2) - 1;
      assert (ind[i] != 0);
      this_cuts_num++;
    }
    else if ((this_cuts_num > cuts_num)
        && (ind[i] != 0))
    {
      ind[i] = 0;
      this_cuts_num--;
    }
  }
} /* }}} void mutate */

static individuum_t *recombine (individuum_t *i0, individuum_t *i1) /* {{{ */
{
  individuum_t *offspring;
  int this_cuts_num;
  int i;

  if ((i0 == NULL) || (i1 == NULL))
    return (NULL);

  offspring = malloc (sizeof (*offspring) * inputs_num);
  if (offspring == NULL)
    return (NULL);
  memset (offspring, 0, sizeof (*offspring) * inputs_num);

  this_cuts_num = 0;
  for (i = 0; i < this_cuts_num; i++)
  {
    if (sn_bounded_random (0, 1) == 0)
      offspring[i] = i0[i];
    else
      offspring[i] = i1[i];

    if (offspring[i] != 0)
      this_cuts_num++;
  }

  mutate (offspring, this_cuts_num);

  return (offspring);
} /* }}} individuum_t *recombine */

static int create_offspring (void) /* {{{ */
{
  individuum_t *i0;
  individuum_t *i1;
  individuum_t *i2;

  i0 = population_get_random (population);
  i1 = population_get_random (population);

  i2 = recombine (i0, i1);

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

      if ((target_rating > 0) && (rating <= target_rating))
        do_loop++;
    }

    if ((max_iterations > 0) && (iteration_counter >= max_iterations))
      do_loop++;
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

    ind = calloc (inputs_num, sizeof (*ind));
    if (ind == NULL)
    {
      fprintf (stderr, "calloc failed.\n");
      exit (EXIT_FAILURE);
    }
    mutate (ind, /* num cuts = */ 0);

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
