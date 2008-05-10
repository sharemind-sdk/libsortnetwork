/**
 * collectd - src/sn_population.c
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
#include <assert.h>
#include <pthread.h>

#include "sn_population.h"
#include "sn_network.h"
#include "sn_random.h"

struct sn_population_s
{
  uint32_t size;

  sn_network_t *best_network;
  int best_rating;

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

static int brute_force_minimize (sn_network_t *n_orig)
{
  int stage_index;
  int comparator_index;

  printf ("brute_force_minimize: Running full fledged brute force optimization.\n");

  for (stage_index = 0;
      stage_index < SN_NETWORK_STAGE_NUM (n_orig);
      stage_index++)
  {
    sn_stage_t *s_orig;

    s_orig = SN_NETWORK_STAGE_GET (n_orig, stage_index);

    for (comparator_index = 0;
	comparator_index < SN_STAGE_COMP_NUM (s_orig);
	comparator_index++)
    {
      sn_network_t *n_copy;
      sn_stage_t *s_copy;
      int status;

      n_copy = sn_network_clone (n_orig);
      if (n_copy == NULL)
	continue;

      s_copy = SN_NETWORK_STAGE_GET (n_copy, stage_index);
      sn_stage_comparator_remove (s_copy, comparator_index);

      status = sn_network_brute_force_check (n_copy);
      if (status == 0)
      {
	printf ("brute_force_minimize: Removed a comparator "
	    "(stage %i, comparator %i).\n",
	    stage_index, comparator_index);
	sn_stage_comparator_remove (s_orig, comparator_index);

	comparator_index--;
      }

      sn_network_destroy (n_copy);
      n_copy = NULL;
    } /* for (comparator_index) */
  } /* for (stage_index) */

  /* FIXME: Remove this check after debugging. */
  assert (sn_network_brute_force_check (n_orig) == 0);
  
  return (0);
} /* int brute_force_minimize */

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

  if (p->best_rating >= rating)
  {
    pthread_mutex_unlock (&p->lock);

    brute_force_minimize (n_copy);
    rating = rate_network (n_copy);

    pthread_mutex_lock (&p->lock);
  }

  if ((p->best_rating >= rating)
      || (p->best_network == NULL))
  {
    sn_network_t *n_copy_best;

    n_copy_best = sn_network_clone (n);
    if (n_copy_best != NULL)
    {
      if (p->best_network != NULL)
	sn_network_destroy (p->best_network);
      p->best_network = n_copy_best;
      p->best_rating = rating;
    }
  }

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

int sn_population_best_rating (sn_population_t *p)
{
  uint32_t i;
  int rating = -1;

  pthread_mutex_lock (&p->lock);

  if (p->networks_num <= 0)
  {
    pthread_mutex_unlock (&p->lock);
    return (-1);
  }

  for (i = 0; i < p->networks_num; i++)
    if ((p->ratings[i] < rating) || (rating < 0))
      rating = p->ratings[i];

  pthread_mutex_unlock (&p->lock);

  return (rating);
} /* int sn_population_best_rating */

/* vim: set shiftwidth=2 softtabstop=2 : */
