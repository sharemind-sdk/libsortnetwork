#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "sn_population.h"
#include "sn_network.h"
#include "sn_random.h"

struct sn_population_s
{
  uint32_t size;

  sn_network_t **networks;
  int *ratings;
  uint32_t networks_num;

  pthread_mutex_t lock;
};

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

sn_population_t *sn_population_create (uint32_t size)
{
  sn_population_t *p;
  int status;

  p = (sn_population_t *) malloc (sizeof (sn_population_t));
  if (p == NULL)
    return (NULL);
  memset (p, 0, sizeof (sn_population_t));
  p->size = size;

  p->networks = (sn_network_t **) calloc ((size_t) size,
      sizeof (sn_network_t *));
  if (p->networks == NULL)
  {
    free (p);
    return (NULL);
  }

  p->ratings = (int *) calloc ((size_t) size, sizeof (int));
  if (p->ratings == NULL)
  {
    free (p->networks);
    free (p);
    return (NULL);
  }

  status = pthread_mutex_init (&p->lock, /* attr = */ NULL);
  if (status != 0)
  {
    fprintf (stderr, "sn_population_create: pthread_mutex_init failed: %i\n",
	status);
    free (p->ratings);
    free (p->networks);
    free (p);
    return (NULL);
  }

  return (p);
} /* sn_population_t *sn_population_create */

void sn_population_destroy (sn_population_t *p)
{
  uint32_t i;

  if (p == NULL)
    return;

  for (i = 0; i < p->size; i++)
    if (p->networks[i] != NULL)
      sn_network_destroy (p->networks[i]);

  pthread_mutex_destroy (&p->lock);

  free (p->ratings);
  free (p->networks);
  free (p);
} /* void sn_population_destroy */

int sn_population_push (sn_population_t *p, sn_network_t *n)
{
  sn_network_t *n_copy;
  int rating;

  n_copy = sn_network_clone (n);
  if (n_copy == NULL)
  {
    fprintf (stderr, "sn_population_push: sn_network_clone failed.\n");
    return (-1);
  }

  rating = rate_network (n_copy);

  pthread_mutex_lock (&p->lock);

  if (p->networks_num < p->size)
  {
    p->networks[p->networks_num] = n_copy;
    p->ratings[p->networks_num] = rating;
    p->networks_num++;
  }
  else
  {
    int index;
    sn_network_t *n_removed = n_copy;
    int i;

    for (i = 0; i < (1 + (p->networks_num / 16)); i++)
    {
      index = sn_bounded_random (0, p->networks_num - 1);
      if (p->ratings[index] < rating)
	continue;

      n_removed = p->networks[index];
      p->networks[index] = n_copy;
      p->ratings[index] = rating;
      break;
    }

    sn_network_destroy (n_removed);
  }

  pthread_mutex_unlock (&p->lock);

  return (0);
} /* int sn_population_push */

sn_network_t *sn_population_pop (sn_population_t *p)
{
  int index;
  sn_network_t *n_copy;

  pthread_mutex_lock (&p->lock);

  if (p->networks_num <= 0)
  {
    pthread_mutex_unlock (&p->lock);
    return (NULL);
  }

  index = sn_bounded_random (0, p->networks_num - 1);
  n_copy = sn_network_clone (p->networks[index]);

  pthread_mutex_unlock (&p->lock);

  return (n_copy);
} /* sn_population_t *sn_population_pop */

sn_network_t *sn_population_best (sn_population_t *p)
{
  uint32_t i;

  uint32_t index = 0;
  int rating = -1;
  sn_network_t *n_copy;

  if (p == NULL)
    return (NULL);

  pthread_mutex_lock (&p->lock);

  if (p->networks_num <= 0)
  {
    pthread_mutex_unlock (&p->lock);
    return (NULL);
  }

  for (i = 0; i < p->networks_num; i++)
  {
    if ((p->ratings[i] < rating) || (rating < 0))
    {
      rating = p->ratings[i];
      index = i;
    }
  }

  n_copy = sn_network_clone (p->networks[index]);

  pthread_mutex_unlock (&p->lock);

  return (n_copy);
} /* sn_network_t *sn_population_best */

/* vim: set shiftwidth=2 softtabstop=2 : */