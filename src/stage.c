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

#include "stage.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "comparator.h"


sn_stage_t *sn_stage_create (int depth)
{
  sn_stage_t *s;

  s = (sn_stage_t *) malloc (sizeof (sn_stage_t));
  if (s == NULL)
    return (NULL);

  s->m_depth = depth;
  s->m_comparators = NULL;
  s->m_comparators_num = 0;
  
  return (s);
} /* sn_stage_t *sn_stage_create */

void sn_stage_destroy (sn_stage_t *s)
{
  if (s == NULL)
    return;
  if (s->m_comparators != NULL)
    free (s->m_comparators);
  free (s);
} /* void sn_stage_destroy */

int sn_stage_sort (sn_stage_t *s, int *values)
{
  sn_comparator_t *c;
  int i;

  for (i = 0; i < s->m_comparators_num; i++)
  {
    c = s->m_comparators + i;
    int const min = SN_COMP_MIN(c);
    int const max = SN_COMP_MAX(c);
    if (values[min] > values[max])
    {
      int temp;
      
      temp = values[min];
      values[min] = values[max];
      values[max] = temp;
    }
  }

  return (0);
} /* int sn_stage_sort */

int sn_stage_comparator_add (sn_stage_t *s, const sn_comparator_t *c)
{
  sn_comparator_t *temp;
  int i;

  if ((s == NULL) || (c == NULL))
    return (EINVAL);

  i = sn_stage_comparator_check_conflict (s, c);
  if (i != 0)
    return (i);

  temp = (sn_comparator_t *) realloc (s->m_comparators,
      (s->m_comparators_num + 1) * sizeof (sn_comparator_t));
  if (temp == NULL)
    return (ENOMEM);
  s->m_comparators = temp;
  temp = NULL;

  for (i = 0; i < s->m_comparators_num; i++)
    if (sn_comparator_compare (c, s->m_comparators + i) <= 0)
      break;

  /* Insert the elements in ascending order */
  assert (i <= s->m_comparators_num);
  if (i < s->m_comparators_num)
  {
    int nmemb = s->m_comparators_num - i;
    memmove (s->m_comparators + (i + 1), s->m_comparators + i,
        nmemb * sizeof (sn_comparator_t));
  }
  memcpy (s->m_comparators + i, c, sizeof (sn_comparator_t));
  s->m_comparators_num++;

  return (0);
} /* int sn_stage_comparator_add */

int sn_stage_comparator_remove (sn_stage_t *s, int c_num)
{
  int nmemb = s->m_comparators_num - (c_num + 1);
  sn_comparator_t *temp;

  if ((s == NULL) || (s->m_comparators_num <= c_num))
    return (EINVAL);

  assert (c_num < s->m_comparators_num);
  assert (c_num >= 0);

  if (nmemb > 0)
    memmove (s->m_comparators + c_num, s->m_comparators + (c_num + 1),
        nmemb * sizeof (sn_comparator_t));
  s->m_comparators_num--;

  /* Free the unused memory */
  if (s->m_comparators_num == 0)
  {
    free (s->m_comparators);
    s->m_comparators = NULL;
  }
  else
  {
    temp = (sn_comparator_t *) realloc (s->m_comparators,
    s->m_comparators_num * sizeof (sn_comparator_t));
    if (temp == NULL)
      return (-1);
    s->m_comparators = temp;
  }

  return (0);
} /* int sn_stage_comparator_remove */

sn_stage_t *sn_stage_clone (const sn_stage_t *s)
{
  sn_stage_t *s_copy;
  int i;

  s_copy = sn_stage_create (s->m_depth);
  if (s_copy == NULL)
    return (NULL);

  s_copy->m_comparators = (sn_comparator_t *) malloc (s->m_comparators_num
      * sizeof (sn_comparator_t));
  if (s_copy->m_comparators == NULL)
  {
    free (s_copy);
    return (NULL);
  }

  for (i = 0; i < s->m_comparators_num; i++)
  {
    SN_COMP_MIN (s_copy->m_comparators + i) = SN_COMP_MIN (s->m_comparators + i);
    SN_COMP_MAX (s_copy->m_comparators + i) = SN_COMP_MAX (s->m_comparators + i);
  }
  s_copy->m_comparators_num = s->m_comparators_num;

  return (s_copy);
} /* sn_stage_t *sn_stage_clone */

int sn_stage_comparator_check_conflict (sn_stage_t *s, const sn_comparator_t *c0)
{
  int i;

  for (i = 0; i < s->m_comparators_num; i++)
  {
    sn_comparator_t *c1 = s->m_comparators + i;
    if ((SN_COMP_MIN(c0) == SN_COMP_MIN(c1))
        || (SN_COMP_MIN(c0) == SN_COMP_MAX(c1))
        || (SN_COMP_MAX(c0) == SN_COMP_MIN(c1))
        || (SN_COMP_MAX(c0) == SN_COMP_MAX(c1)))
    {
      if ((SN_COMP_MIN(c0) == SN_COMP_MIN(c1))
          && (SN_COMP_MAX(c0) == SN_COMP_MAX(c1)))
        return (2);
      else
        return (1);
    }
  }

  return (0);
} /* int sn_stage_comparator_check_conflict */

void sn_stage_invert (sn_stage_t *s)
{
  assert(s);
  for (int i = 0; i < s->m_comparators_num; ++i)
    sn_comparator_invert (s->m_comparators + i);
} /* void sn_stage_invert */

int sn_stage_shift (sn_stage_t *s, int sw, int inputs_num)
{
  int i;

  if ((s == NULL) || (inputs_num < 2))
    return (EINVAL);

  sw %= inputs_num;
  if (sw == 0)
    return (0);

  for (i = 0; i < s->m_comparators_num; i++)
    sn_comparator_shift (s->m_comparators + i, sw, inputs_num);

  return (0);
} /* int sn_stage_shift */

static int sn_stage_unify__qsort_callback (const void *p0, const void *p1) /* {{{ */
{
    return sn_comparator_compare((sn_comparator_t const *) p0,
                                 (sn_comparator_t const *) p1);
} /* }}} int sn_stage_unify__qsort_callback */

int sn_stage_unify (sn_stage_t *s) /* {{{ */
{
  if (s == NULL)
    return (EINVAL);

  qsort (s->m_comparators,
      (size_t) s->m_comparators_num,
      sizeof (*s->m_comparators),
      sn_stage_unify__qsort_callback);

  return (0);
} /* }}} int sn_stage_unify */

int sn_stage_swap (sn_stage_t *s, int con0, int con1)
{
  int i;

  if (s == NULL)
    return (EINVAL);

  for (i = 0; i < s->m_comparators_num; i++)
    sn_comparator_swap (s->m_comparators + i, con0, con1);

  return (0);
} /* int sn_stage_swap */

int sn_stage_cut_at (sn_stage_t *s, int input, enum sn_network_cut_dir_e dir)
{
  int new_position = input;
  int i;

  if ((s == NULL) || (input < 0))
    return (-EINVAL);

  for (i = 0; i < s->m_comparators_num; i++)
  {
    sn_comparator_t *c = s->m_comparators + i;

    if ((SN_COMP_MIN (c) != input) && (SN_COMP_MAX (c) != input))
      continue;

    if ((dir == DIR_MIN) && (SN_COMP_MAX (c) == input))
    {
      new_position = SN_COMP_MIN (c);
    }
    else if ((dir == DIR_MAX) && (SN_COMP_MIN (c) == input))
    {
      new_position = SN_COMP_MAX (c);
    }
    break;
  }

  /*
  if (i < s->comparators_num)
    sn_stage_comparator_remove (s, i);
    */

  return (new_position);
} /* int sn_stage_cut_at */

int sn_stage_cut (sn_stage_t *s, int *mask, /* {{{ */
    sn_stage_t **prev)
{
  int i;

  if ((s == NULL) || (mask == NULL) || (prev == NULL))
    return (EINVAL);

  for (i = 0; i < s->m_comparators_num; i++)
  {
    sn_comparator_t *c = s->m_comparators + i;
    int left = SN_COMP_LEFT (c);
    int right = SN_COMP_RIGHT (c);

    if ((mask[left] == 0)
        && (mask[right] == 0))
      continue;

    /* Check if we need to update the cut position */
    if ((mask[left] != mask[right])
        && ((mask[left] > 0) || (mask[right] < 0)))
    {
      int tmp;
      int j;

      tmp = mask[right];
      mask[right] = mask[left];
      mask[left] = tmp;

      for (j = s->m_depth - 1; j >= 0; j--)
        sn_stage_swap (prev[j],
            left, right);
    }

    sn_stage_comparator_remove (s, i);
    i--;
  } /* for (i = 0; i < s->comparators_num; i++) */

  return (0);
} /* }}} int sn_stage_cut */

int sn_stage_remove_input (sn_stage_t *s, int input)
{
  int i;

  for (i = 0; i < s->m_comparators_num; i++)
  {
    sn_comparator_t *c = s->m_comparators + i;

    if ((SN_COMP_MIN (c) == input) || (SN_COMP_MAX (c) == input))
    {
      sn_stage_comparator_remove (s, i);
      i--;
      continue;
    }

    if (SN_COMP_MIN (c) > input)
      SN_COMP_MIN (c) = SN_COMP_MIN (c) - 1;
    if (SN_COMP_MAX (c) > input)
      SN_COMP_MAX (c) = SN_COMP_MAX (c) - 1;
  }

  return (0);
}

int sn_stage_compare (const sn_stage_t *s0, const sn_stage_t *s1) /* {{{ */
{
  int status;
  int i;

  if (s0 == s1)
    return (0);
  else if (s0 == NULL)
    return (-1);
  else if (s1 == NULL)
    return (1);

  if (s0->m_comparators_num < s1->m_comparators_num)
    return (-1);
  else if (s0->m_comparators_num > s1->m_comparators_num)
    return (1);

  for (i = 0; i < s0->m_comparators_num; i++)
  {
    status = sn_comparator_compare (s0->m_comparators + i, s1->m_comparators + i);
    if (status != 0)
      return (status);
  }

  return (0);
} /* }}} int sn_stage_compare */
