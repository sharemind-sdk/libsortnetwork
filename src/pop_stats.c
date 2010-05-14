/**
 * libsortnetwork - src/pop_stats.c
 * Copyright (C) 2009-2010  Florian octo Forster
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "pop_stats.h"

/* Yes, this is ugly, but the GNU libc doesn't export it with the above flags.
 * */
char *strdup (const char *s);

struct pop_stats_s /* {{{ */
{
  pthread_mutex_t lock;

  /* Options */
  char *opt_file;
  long opt_interval;

  /* Data */
  long count;
  int64_t rating_sum;
  int rating_worst;
  int rating_best;
}; /* }}} struct pop_stats_s */

/*
 * Private functions
 */
static int ps_flush (pop_stats_t *ps) /* {{{ */
{
  double average;

  average = ((double) ps->rating_sum) / ((double) ps->count);

  fprintf (stdout, "[STATS] worst:%i average:%g best:%i\n",
      ps->rating_worst, average, ps->rating_best);

  ps->count = 0;
  ps->rating_sum = 0;
  ps->rating_worst = 0;
  ps->rating_best = 0;

  return (0);
} /* }}} int ps_flush */

/*
 * Public functions
 */
pop_stats_t *pop_stats_create (void) /* {{{ */
{
  pop_stats_t *ps;

  ps = malloc (sizeof (*ps));
  if (ps == NULL)
    return (NULL);

  memset (ps, 0, sizeof (*ps));
  pthread_mutex_init (&ps->lock, /* attr = */ NULL);

  return (ps);
} /* }}} pop_stats_t *pop_stats_create */

void pop_stats_destroy (pop_stats_t *ps) /* {{{ */
{
  if (ps == NULL)
    return;

  if (ps->count > 0)
    ps_flush (ps);

  free (ps->opt_file);
  free (ps);
} /* }}} void pop_stats_destroy */

int pop_stats_opt_file (pop_stats_t *ps, const char *file) /* {{{ */
{
  char *file_copy;

  if ((ps == NULL) || (file == NULL))
    return (-EINVAL);

  file_copy = strdup (file);
  if (file_copy == NULL)
    return (-ENOMEM);

  if (ps->opt_file != NULL)
    free (ps->opt_file);
  ps->opt_file = file_copy;

  return (0);
} /* }}} int pop_stats_opt_file */

int pop_stats_opt_interval (pop_stats_t *ps, long interval) /* {{{ */
{
  if ((ps == NULL) || (interval <= 0))
    return (-EINVAL);

  ps->opt_interval = interval;

  return (0);
} /* }}} int pop_stats_opt_interval */

int pop_stats_add_rating (pop_stats_t *ps, int rating) /* {{{ */
{
  if (ps == NULL)
    return (-EINVAL);

  pthread_mutex_lock (&ps->lock);

  if (ps->count == 0)
  {
    ps->rating_worst = rating;
    ps->rating_best = rating;
  }
  else
  {
    if (ps->rating_worst < rating)
      ps->rating_worst = rating;
    if (ps->rating_best > rating)
      ps->rating_best = rating;
  }

  ps->rating_sum += ((int64_t) rating);
  ps->count++;

  if (ps->count >= ps->opt_interval)
    ps_flush (ps);

  pthread_mutex_unlock (&ps->lock);
  return (0);
} /* }}} int pop_stats_add_rating */

/* vim: set shiftwidth=2 softtabstop=2 et fdm=marker : */
