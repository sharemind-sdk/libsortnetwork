/**
 * libsortnetwork - src/sn_comparator.c
 * Copyright (C) 2008-2010  Florian octo Forster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "comparator.h"

sn_comparator_t *sn_comparator_create (int min, int max)
{
  sn_comparator_t *c;

  c = (sn_comparator_t *) malloc (sizeof (sn_comparator_t));
  if (c == NULL)
    return (NULL);
  memset (c, '\0', sizeof (sn_comparator_t));

  c->min = min;
  c->max = max;
  c->user_data = NULL;
  c->free_func = NULL;

  return (c);
} /* sn_comparator_t *sn_comparator_create */

void sn_comparator_destroy (sn_comparator_t *c)
{
  if (c->free_func != NULL)
    c->free_func (c->user_data);

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

int sn_comparator_compare (const sn_comparator_t *c0,
    const sn_comparator_t *c1)
{
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

uint64_t sn_comparator_get_hashval (const sn_comparator_t *c) /* {{{ */
{
  if (c == NULL)
    return (0);

  /* 100937 and 103319 are some random prime numbers */
  return ((((uint64_t) c->min) * 100937)
      + (((uint64_t) c->max) * 103319));
} /* }}} uint32_t sn_comparator_get_hashval */

/* vim: set shiftwidth=2 softtabstop=2 : */