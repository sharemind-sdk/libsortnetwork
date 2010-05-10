/**
 * collectd - src/sn_comparator.c
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
#include <string.h>

#include "sn_comparator.h"

sn_comparator_t *sn_comparator_create (int min, int max)
{
  sn_comparator_t *c;

  c = (sn_comparator_t *) malloc (sizeof (sn_comparator_t));
  if (c == NULL)
    return (NULL);
  memset (c, '\0', sizeof (sn_comparator_t));

  c->min = min;
  c->max = max;

  return (c);
} /* sn_comparator_t *sn_comparator_create */

void sn_comparator_destroy (sn_comparator_t *c)
{
  if (c != NULL)
    free (c);
} /* void sn_comparator_destroy */

void sn_comparator_invert (sn_comparator_t *c)
{
  int max = c->min;
  int min = c->max;

  c->min = min;
  c->max = max;
} /* void sn_comparator_invert */

void sn_comparator_shift (sn_comparator_t *c, int sw, int inputs_num)
{
  c->min = (c->min + sw) % inputs_num;
  c->max = (c->max + sw) % inputs_num;
} /* void sn_comparator_shift */

void sn_comparator_swap (sn_comparator_t *c, int con0, int con1)
{
  if (c->min == con0)
  {
    c->min = con1;
  }
  else if (c->min == con1)
  {
    c->min = con0;
  }

  if (c->max == con0)
  {
    c->max = con1;
  }
  else if (c->max == con1)
  {
    c->max = con0;
  }
} /* void sn_comparator_swap */

int sn_comparator_compare (const void *v0, const void *v1)
{
  sn_comparator_t *c0 = (sn_comparator_t *) v0;
  sn_comparator_t *c1 = (sn_comparator_t *) v1;

  if (SN_COMP_LEFT (c0) < SN_COMP_LEFT (c1))
    return (-1);
  else if (SN_COMP_LEFT (c0) > SN_COMP_LEFT (c1))
    return (1);
  else if (SN_COMP_RIGHT (c0) < SN_COMP_RIGHT (c1))
    return (-1);
  else if (SN_COMP_RIGHT (c0) > SN_COMP_RIGHT (c1))
    return (1);
  else
    return (0);
} /* int sn_comparator_compare */

/* vim: set shiftwidth=2 softtabstop=2 : */
