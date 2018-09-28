/*
 * Copyright (C) 2008-2010  Florian octo Forster
 * Copyright (C) 2017-2018  Cybernetica AS
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
 * Modified by Cybernetica AS <sharemind-support at cyber.ee>
 */

#include "comparator.h"

#include <assert.h>
#include <stdlib.h>



void sn_comparator_init(sn_comparator_t * c, int min, int max) {
  assert(c);
  c->m_min = min;
  c->m_max = max;
}

sn_comparator_t *sn_comparator_create (int min, int max)
{
  sn_comparator_t *c;

  c = (sn_comparator_t *) malloc (sizeof (sn_comparator_t));
  if (c == NULL)
    return (NULL);
  sn_comparator_init(c, min, max);
  return (c);
} /* sn_comparator_t *sn_comparator_create */

void sn_comparator_destroy (sn_comparator_t *c)
{
  assert(c);
  free(c);
} /* void sn_comparator_destroy */

void sn_comparator_invert (sn_comparator_t *c)
{
  assert(c);
  int const tmp = c->m_min;
  c->m_min = c->m_max;
  c->m_max = tmp;
} /* void sn_comparator_invert */

void sn_comparator_shift (sn_comparator_t *c, int sw, int inputs_num)
{
  assert(c);
  c->m_min = (c->m_min + sw) % inputs_num;
  c->m_max = (c->m_max + sw) % inputs_num;
} /* void sn_comparator_shift */

void sn_comparator_swap (sn_comparator_t *c, int con0, int con1)
{
  assert(c);
  if (c->m_min == con0)
  {
    c->m_min = con1;
  }
  else if (c->m_min == con1)
  {
    c->m_min = con0;
  }

  if (c->m_max == con0)
  {
    c->m_max = con1;
  }
  else if (c->m_max == con1)
  {
    c->m_max = con0;
  }
} /* void sn_comparator_swap */

int sn_comparator_compare (const sn_comparator_t *c0,
    const sn_comparator_t *c1)
{
  assert(c0);
  assert(c1);
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
