/**
 * libsortnetwork - src/histrogram.c
 * Copyright (C) 2011  Florian octo Forster
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
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "histogram.h"

#define BAR_WIDTH 64

struct histogram_s
{
  uint64_t *data;
  int index_min;
  int index_max;
};

static int hist_resize (histogram_t *h, int rating) /* {{{ */
{
  int min_new;
  int max_new;
  size_t nelem_new;
  size_t nelem_old;
  uint64_t *tmp;

  if ((h->index_min == 0) && (h->index_max == 0))
  {
    min_new = rating;
    max_new = rating;
  }
  else
  {
    min_new = ((h->index_min < rating) ? h->index_min : rating);
    max_new = ((h->index_max > rating) ? h->index_max : rating);
  }

  nelem_new = (size_t) ((max_new - min_new) + 1);
  nelem_old = (size_t) ((h->index_max - h->index_min) + 1);

  assert (nelem_new >= nelem_old);

  tmp = realloc (h->data, nelem_new * sizeof (*h->data));
  if (tmp == NULL)
    return (ENOMEM);
  h->data = tmp;

  if ((h->index_min == 0) && (h->index_max == 0))
  {
    h->index_min = min_new;
    h->index_max = max_new;
    h->data[0] = 0;
    return (0);
  }

  if (min_new < h->index_min)
  {
    size_t diff = (size_t) (h->index_min - min_new);

    memmove (h->data + diff, h->data, nelem_old * sizeof (*h->data));
    memset (h->data, 0, diff * sizeof (*h->data));

    h->index_min = min_new;
  }
  else if (max_new > h->index_max)
  {
    size_t diff = (size_t) (max_new - h->index_max);
    memset (h->data + nelem_old, 0, diff * sizeof (*h->data));

    h->index_max = max_new;
  }

  return (0);
} /* }}} int hist_resize */

histogram_t *hist_create (void) /* {{{ */
{
  histogram_t *h = malloc (sizeof (*h));
  if (h == NULL)
    return (NULL);
  memset (h, 0, sizeof (*h));
  h->data = NULL;

  return (h);
} /* }}} hist_create */

void hist_destroy (histogram_t *h) /* {{{ */
{
  if (h != NULL)
    free (h->data);
  free (h);
} /* }}} hist_destroy */

int hist_account (histogram_t *h, int rating) /* {{{ */
{
  if ((h == NULL) || (rating < 0))
    return (EINVAL);

  if ((rating < h->index_min) || (rating > h->index_max))
    hist_resize (h, rating);

  h->data[rating - h->index_min]++;

  return (0);
} /* }}} int hist_account */

int hist_print (histogram_t *h) /* {{{ */
{
  char bar[BAR_WIDTH + 1];
  uint64_t max;
  int i;

  if (h == NULL)
    return (EINVAL);

  max = 1;
  for (i = h->index_min; i <= h->index_max; i++)
    if (max < h->data[i - h->index_min])
      max = h->data[i - h->index_min];

  for (i = h->index_min; i <= h->index_max; i++)
  {
    uint64_t num = h->data[i - h->index_min];
    uint64_t points = (BAR_WIDTH * num) / max;
    uint64_t j;

    for (j = 0; j < (BAR_WIDTH + 1); j++)
      bar[j] = (j < points) ? '#' : 0;

    printf ("%4i: %8"PRIu64" %s\n", i, num, bar);
  }

  return (0);
} /* }}} int hist_print */

/* vim: set shiftwidth=2 softtabstop=2 fdm=marker : */
