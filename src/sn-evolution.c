#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>

#include "sn_network.h"

struct population_entry_s
{
  sn_network_t *network;
  int rating;
};
typedef struct population_entry_s population_entry_t;

static int iterations_num  = INT_MAX;
static int max_population_size = 128;
static int olymp_size = 96;
static int inputs_num      = 16;

static population_entry_t *population = NULL;
int population_size = 0;

static void sigint_handler (int signal)
{
  iterations_num = 0;
} /* void sigint_handler */

static int init_random (void)
{
  int fd;
  unsigned int r;

  fd = open ("/dev/random", O_RDONLY);
  if (fd < 0)
  {
    perror ("open");
    return (-1);
  }

  read (fd, (void *) &r, sizeof (r));
  close (fd);

  srand (r);

  return (0);
} /* int init_random */

static int bounded_random (int upper_bound)
{
  double r = ((double) rand ()) / ((double) RAND_MAX);
  return ((int) (r * upper_bound));
}

static void exit_usage (const char *name)
{
  printf ("%s <file0>\n", name);
  exit (1);
} /* void exit_usage */

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

static int insert_into_population (sn_network_t *n)
{
  int rating;
  int worst_rating;
  int worst_index;
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
  for (i = 0; i < olymp_size; i++)
    if (population[i].rating > worst_rating)
    {
      worst_rating = population[i].rating;
      worst_index  = i;
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

static int create_offspring (void)
{
  int p0;
  int p1;
  sn_network_t *n;

  p0 = bounded_random (population_size);
  p1 = bounded_random (population_size);

  n = sn_network_combine (population[p0].network, population[p1].network);

  while (SN_NETWORK_INPUT_NUM (n) > inputs_num)
  {
    int pos;
    enum sn_network_cut_dir_e dir;

    pos = bounded_random (SN_NETWORK_INPUT_NUM (n));
    dir = (bounded_random (2) == 0) ? DIR_MIN : DIR_MAX;

    assert ((pos >= 0) && (pos < SN_NETWORK_INPUT_NUM (n)));

    sn_network_cut_at (n, pos, dir);
  }

  sn_network_compress (n);

  assert (SN_NETWORK_INPUT_NUM (n) == inputs_num);

  insert_into_population (n);

  return (0);
} /* int create_offspring */

static int start_evolution (void)
{
  int i;

  for (i = 0; i < iterations_num; i++)
  {
    if ((i % 1000) == 0)
      population_print_stats (i);

    create_offspring ();
  }

  return (0);
} /* int start_evolution */

int main (int argc, char **argv)
{
  struct sigaction sigint_action;

  if (argc != 2)
    exit_usage (argv[0]);

  init_random ();

  memset (&sigint_action, '\0', sizeof (sigint_action));
  sigint_action.sa_handler = sigint_handler;
  sigaction (SIGINT, &sigint_action, NULL);

  population = (population_entry_t *) malloc (max_population_size
      * sizeof (population_entry_t));
  if (population == NULL)
  {
    printf ("Malloc failed.\n");
    return (-1);
  }
  memset (population, '\0', max_population_size
      * sizeof (population_entry_t));

  {
    sn_network_t *n = sn_network_read_file (argv[1]);
    if (n == NULL)
    {
      printf ("n == NULL\n");
      return (1);
    }
    population[0].network = n;
    population[0].rating  = rate_network (n);
    population_size++;
  }

  start_evolution ();

  {
    int i;
    int best_rate = -1;
    int best_index = -1;

    for (i = 0; i < population_size; i++)
    {
      if ((best_rate == -1) || (best_rate > population[i].rating))
      {
	best_rate = population[i].rating;
	best_index = i;
      }
    }

    sn_network_show (population[best_index].network);
  }

  {
    int i;

    for (i = 0; i < population_size; i++)
    {
      sn_network_destroy (population[i].network);
      population[i].network = NULL;
    }

    free (population);
    population = 0;
  }

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
