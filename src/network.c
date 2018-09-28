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

#include "network.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


sn_network_t *sn_network_create (int inputs_num) /* {{{ */
{
  sn_network_t *n;

  n = (sn_network_t *) malloc (sizeof (sn_network_t));
  if (n == NULL)
    return (NULL);

  n->m_inputs_num = inputs_num;
  n->m_stages = NULL;
  n->m_stages_num = 0;

  return (n);
} /* }}} sn_network_t *sn_network_create */

void sn_network_destroy (sn_network_t *n) /* {{{ */
{
  if (n == NULL)
    return;

  if (n->m_stages != NULL)
  {
    int i;
    for (i = 0; i < n->m_stages_num; i++)
    {
      sn_stage_destroy (n->m_stages[i]);
      n->m_stages[i] = NULL;
    }
    free (n->m_stages);
    n->m_stages = NULL;
  }

  free (n);
} /* }}} void sn_network_destroy */

sn_network_t *sn_network_create_odd_even_mergesort (int inputs_num) /* {{{ */
{
  sn_network_t *n;

  assert (inputs_num > 0);
  if (inputs_num == 1)
  {
    return (sn_network_create (inputs_num));
  }
  if (inputs_num == 2)
  {
    n = sn_network_create (inputs_num);

    sn_comparator_t c;
    sn_comparator_init(&c, 0, 1);
    sn_network_comparator_add (n, &c);

    return (n);
  }
  else
  {
    sn_network_t *n_left;
    sn_network_t *n_right;
    int inputs_left;
    int inputs_right;

    inputs_left = inputs_num / 2;
    inputs_right = inputs_num - inputs_left;

    n_left = sn_network_create_odd_even_mergesort (inputs_left);
    if (n_left == NULL)
      return (NULL);

    n_right = sn_network_create_odd_even_mergesort (inputs_right);
    if (n_right == NULL)
    {
      sn_network_destroy (n_left);
      return (NULL);
    }

    n = sn_network_combine_odd_even_merge (n_left, n_right);

    sn_network_destroy (n_left);
    sn_network_destroy (n_right);

    if (n != NULL)
      sn_network_compress (n);

    return (n);
  }
} /* }}} sn_network_t *sn_network_create_odd_even_mergesort */

sn_network_t *sn_network_create_bitonic_mergesort (int inputs_num) /* {{{ */
{
  sn_network_t *n;

  assert (inputs_num > 0);
  if (inputs_num == 1)
  {
    return (sn_network_create (inputs_num));
  }
  if (inputs_num == 2)
  {
    n = sn_network_create (inputs_num);

    sn_comparator_t c;
    sn_comparator_init(&c, 0, 1);
    sn_network_comparator_add (n, &c);

    return (n);
  }
  else
  {
    sn_network_t *n_left;
    sn_network_t *n_right;
    int inputs_left;
    int inputs_right;

    inputs_left = inputs_num / 2;
    inputs_right = inputs_num - inputs_left;

    n_left = sn_network_create_bitonic_mergesort (inputs_left);
    if (n_left == NULL)
      return (NULL);

    if (inputs_left != inputs_right)
      n_right = sn_network_create_bitonic_mergesort (inputs_right);
    else
      n_right = n_left;
    if (n_right == NULL)
    {
      sn_network_destroy (n_left);
      return (NULL);
    }

    n = sn_network_combine_bitonic_merge (n_left, n_right);

    if (n_left != n_right)
      sn_network_destroy (n_right);
    sn_network_destroy (n_left);

    if (n != NULL)
      sn_network_compress (n);

    return (n);
  }
} /* }}} sn_network_t *sn_network_create_bitonic_mergesort */

static int sn_network_create_pairwise_internal (sn_network_t *n, /* {{{ */
    int *inputs, int inputs_num)
{
  int i;
  int m;

  for (i = 1; i < inputs_num; i += 2)
  {
    sn_comparator_t c;
    sn_comparator_init(&c, inputs[i-1], inputs[i]);
    sn_network_comparator_add (n, &c);
  }

  if (inputs_num <= 2)
    return (0);

  if (SIZE_MAX / sizeof(int) < (size_t) inputs_num)
      return (ENOMEM);
  int * inputs_copy = (int *) malloc(sizeof(int) * (size_t) inputs_num);
  if (!inputs_copy)
      return (ENOMEM);

  /* Sort "pairs" recursively. Like with odd-even mergesort, odd and even lines
   * are handled recursively and later reunited. */
  for (i = 0; i < inputs_num; i += 2)
    inputs_copy[(int) (i / 2)] = inputs[i];
  /* Recursive call #1 with first set of lines */
  sn_network_create_pairwise_internal (n, inputs_copy,
      (int) ((inputs_num + 1) / 2));

  for (i = 1; i < inputs_num; i += 2)
    inputs_copy[(int) (i / 2)] = inputs[i];
  /* Recursive call #2 with second set of lines */
  sn_network_create_pairwise_internal (n, inputs_copy,
      (int) (inputs_num / 2));

  /* m is the "amplitude" of the sorted pairs. This is a bit tricky to read due
   * to different indices being used in the paper, unfortunately. */
  m = (inputs_num + 1) / 2;
  while (m > 1)
  {
    int len;

    /* len = ((int) ((m + 1) / 2)) * 2 - 1; */
    if ((m % 2) == 0)
      len = m - 1;
    else
      len = m;

    for (i = 1; (i + len) < inputs_num; i += 2)
    {
      int left = i;
      int right = i + len;

      assert (left < right);
      sn_comparator_t c;
      sn_comparator_init(&c, inputs[left], inputs[right]);
      sn_network_comparator_add(n, &c);
    }

    m = (m + 1) / 2;
  } /* while (m > 1) */

  free(inputs_copy);
  return (0);
} /* }}} int sn_network_create_pairwise_internal */

sn_network_t *sn_network_create_pairwise (int inputs_num) /* {{{ */
{
  sn_network_t *n = sn_network_create (inputs_num);
  int * inputs;
  int i;

  if (n == NULL)
    goto err_0;

  if (SIZE_MAX / sizeof(int) < inputs_num)
    goto err_1;
  inputs = (int *) malloc(sizeof(int) * inputs_num);
  for (i = 0; i < inputs_num; i++)
    inputs[i] = i;

  if (sn_network_create_pairwise_internal (n, inputs, inputs_num))
      goto err_2;

  sn_network_compress (n);

  return (n);

err_2:
  free(inputs);
err_1:
  sn_network_destroy(n);
err_0:
  return NULL;
} /* }}} sn_network_t *sn_network_create_pairwise */

int sn_network_network_add (sn_network_t *n, sn_network_t *other) /* {{{ */
{
  int stages_num;
  int i;

  if ((n == NULL) || (other == NULL))
    return (EINVAL);

  stages_num = n->m_stages_num + other->m_stages_num;
  if (stages_num <= n->m_stages_num)
    return (EINVAL);

  sn_stage_t ** const tmp =
          (sn_stage_t **) realloc(n->m_stages, sizeof(*n->m_stages) * stages_num);
  if (tmp == NULL)
    return (ENOMEM);
  n->m_stages = tmp;

  memcpy (n->m_stages + n->m_stages_num, other->m_stages,
      sizeof (*other->m_stages) * other->m_stages_num);
  for (i = n->m_stages_num; i < stages_num; i++)
    SN_STAGE_DEPTH(n->m_stages[i]) = i;

  n->m_stages_num = stages_num;

  free (other->m_stages);
  free (other);

  return (0);
} /* }}} int sn_network_network_add */

int sn_network_stage_add (sn_network_t *n, sn_stage_t *s) /* {{{ */
{
  sn_stage_t **temp;

  if ((n == NULL) || (s == NULL))
    return (EINVAL);

  temp = (sn_stage_t **) realloc (n->m_stages, (n->m_stages_num + 1)
      * sizeof (sn_stage_t *));
  if (temp == NULL)
    return (-1);

  n->m_stages = temp;
  SN_STAGE_DEPTH (s) = n->m_stages_num;
  n->m_stages[n->m_stages_num] = s;
  n->m_stages_num++;

  return (0);
} /* }}} int sn_network_stage_add */

int sn_network_stage_remove (sn_network_t *n, int s_num) /* {{{ */
{
  int nmemb = n->m_stages_num - (s_num + 1);
  sn_stage_t **temp;

  if ((n == NULL) || (s_num >= n->m_stages_num))
    return (EINVAL);

  sn_stage_destroy (n->m_stages[s_num]);
  n->m_stages[s_num] = NULL;

  if (nmemb > 0)
  {
    memmove (n->m_stages + s_num, n->m_stages + (s_num + 1),
        nmemb * sizeof (sn_stage_t *));
    n->m_stages[n->m_stages_num - 1] = NULL;
  }
  n->m_stages_num--;

  /* Free the unused memory */
  if (n->m_stages_num == 0)
  {
    free (n->m_stages);
    n->m_stages = NULL;
  }
  else
  {
    temp = (sn_stage_t **) realloc (n->m_stages,
        n->m_stages_num * sizeof (sn_stage_t *));
    if (temp == NULL)
      return (-1);
    n->m_stages = temp;
  }

  return (0);
} /* }}} int sn_network_stage_remove */

sn_network_t *sn_network_clone (const sn_network_t *n) /* {{{ */
{
  sn_network_t *n_copy;
  int i;

  n_copy = sn_network_create (n->m_inputs_num);
  if (n_copy == NULL)
    return (NULL);

  for (i = 0; i < n->m_stages_num; i++)
  {
    sn_stage_t *s;
    int status;

    s = sn_stage_clone (n->m_stages[i]);
    if (s == NULL)
      break;

    status = sn_network_stage_add (n_copy, s);
    if (status != 0)
      break;
  }

  if (i < n->m_stages_num)
  {
    sn_network_destroy (n_copy);
    return (NULL);
  }

  return (n_copy);
} /* }}} sn_network_t *sn_network_clone */

int sn_network_comparator_add (sn_network_t *n, /* {{{ */
    const sn_comparator_t *c)
{
  sn_stage_t *s;

  if ((n == NULL) || (c == NULL))
    return (EINVAL);

  if (n->m_stages_num > 0)
  {
    s = n->m_stages[n->m_stages_num - 1];

    if (sn_stage_comparator_check_conflict (s, c) == 0)
    {
      sn_stage_comparator_add (s, c);
      return (0);
    }
  }

  s = sn_stage_create (n->m_stages_num);
  sn_stage_comparator_add (s, c);
  sn_network_stage_add (n, s);

  return (0);
} /* }}} int sn_network_comparator_add */

int sn_network_get_comparator_num (const sn_network_t *n) /* {{{ */
{
  int num;
  int i;

  if (n == NULL)
    return (-1);

  num = 0;
  for (i = 0; i < n->m_stages_num; i++)
    num += SN_STAGE_COMP_NUM(n->m_stages[i]);

  return (num);
} /* }}} int sn_network_get_comparator_num */

void sn_network_invert (sn_network_t *n) /* {{{ */
{
  assert(n);
  for (int i = 0; i < n->m_stages_num; i++)
    sn_stage_invert (n->m_stages[i]);
} /* }}} void sn_network_invert */

int sn_network_shift (sn_network_t *n, int sw) /* {{{ */
{
  int i;

  if ((n == NULL) || (sw < 0))
    return (EINVAL);

  if (sw == 0)
    return (0);

  for (i = 0; i < n->m_stages_num; i++)
    sn_stage_shift (n->m_stages[i], sw, SN_NETWORK_INPUT_NUM (n));

  return (0);
} /* }}} int sn_network_shift */

int sn_network_compress (sn_network_t *n) /* {{{ */
{
  int i;
  int j;
  int k;

  for (i = 1; i < n->m_stages_num; i++)
  {
    sn_stage_t *s;

    s = n->m_stages[i];

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c = SN_STAGE_COMP_GET (s, j);
      int move_to = i;

      for (k = i - 1; k >= 0; k--)
      {
        int conflict;

        conflict = sn_stage_comparator_check_conflict (n->m_stages[k], c);
        if (conflict == 0)
        {
          move_to = k;
          continue;
        }

        if (conflict == 2)
          move_to = -1;
        break;
      }

      if (move_to < i)
      {
        if (move_to >= 0)
          sn_stage_comparator_add (n->m_stages[move_to], c);
        sn_stage_comparator_remove (s, j);
        j--;
      }
    }
  }

  while ((n->m_stages_num > 0)
      && (SN_STAGE_COMP_NUM (n->m_stages[n->m_stages_num - 1]) == 0))
    sn_network_stage_remove (n, n->m_stages_num - 1);

  return (0);
} /* }}} int sn_network_compress */

int sn_network_normalize (sn_network_t *n) /* {{{ */
{
  int i;

  for (i = 0; i < n->m_stages_num; i++)
  {
    sn_stage_t *s;
    int j;

    s = n->m_stages[i];

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c;
      int min;
      int max;

      c = SN_STAGE_COMP_GET (s, j);

      min = SN_COMP_MIN(c);
      max = SN_COMP_MAX(c);

      if (min > max)
      {
        int k;

        for (k = i; k < n->m_stages_num; k++)
          sn_stage_swap (n->m_stages[k], min, max);

        i = -1;
        break; /* for (j) */
      }
    } /* for (j = 0 .. #comparators) */
  } /* for (i = n->m_stages_num - 1 .. 0) */

  return (0);
} /* }}} int sn_network_normalize */

int sn_network_unify (sn_network_t *n) /* {{{ */
{
  int i;

  if (n == NULL)
    return (EINVAL);

  sn_network_normalize (n);
  sn_network_compress (n);

  for (i = 0; i < n->m_stages_num; i++)
    sn_stage_unify (n->m_stages[i]);

  return (0);
} /* }}} int sn_network_unify */

int sn_network_remove_input (sn_network_t *n, int input) /* {{{ */
{
  int i;

  if ((n == NULL) || (input < 0) || (input >= n->m_inputs_num))
    return (EINVAL);

  for (i = 0; i < n->m_stages_num; i++)
    sn_stage_remove_input (n->m_stages[i], input);

  n->m_inputs_num--;

  return (0);
} /* }}} int sn_network_remove_input */

int sn_network_cut_at (sn_network_t *n, int input, /* {{{ */
    enum sn_network_cut_dir_e dir)
{
  int i;
  int position = input;

  for (i = 0; i < n->m_stages_num; i++)
  {
    sn_stage_t *s;
    int new_position;

    s = n->m_stages[i];
    new_position = sn_stage_cut_at (s, position, dir);

    if (position != new_position)
    {
      int j;

      for (j = 0; j < i; j++)
        sn_stage_swap (n->m_stages[j], position, new_position);
    }

    position = new_position;
  }

  assert (((dir == DIR_MIN) && (position == 0))
      || ((dir == DIR_MAX) && (position == (n->m_inputs_num - 1))));

  sn_network_remove_input (n, position);

  return (0);
} /* }}} int sn_network_cut_at */

int sn_network_cut (sn_network_t *n, int *mask) /* {{{ */
{
  int inputs_num;
  int i;

  for (i = 0; i < n->m_stages_num; i++)
  {
    sn_stage_t *s = n->m_stages[i];

    sn_stage_cut (s, mask, n->m_stages);
  }

  /* Use a copy of this member since it will be updated by
   * sn_network_remove_input(). */
  inputs_num = n->m_inputs_num;
  for (i = 0; i < inputs_num; i++)
  {
    if (mask[i] < 0)
      sn_network_remove_input (n, 0);
    else if (mask[i] > 0)
      sn_network_remove_input (n, n->m_inputs_num - 1);
  }

  return (0);
} /* }}} int sn_network_cut */

/* sn_network_concatenate
 *
 * `Glues' two networks together, resulting in a comparator network with twice
 * as many inputs but one that doesn't really sort anymore. It produces a
 * bitonic sequence, though, that can be used by the mergers below. */
static sn_network_t *sn_network_concatenate (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  sn_network_t *n;
  int stages_num;
  int i;
  int j;

  stages_num = (n0->m_stages_num > n1->m_stages_num)
    ? n0->m_stages_num
    : n1->m_stages_num;

  n = sn_network_create (n0->m_inputs_num + n1->m_inputs_num);
  if (n == NULL)
    return (NULL);

  for (i = 0; i < stages_num; i++)
  {
    sn_stage_t *s = sn_stage_create (i);

    if (i < n0->m_stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n0->m_stages[i]); j++)
      {
        sn_comparator_t *c = SN_STAGE_COMP_GET (n0->m_stages[i], j);
        sn_stage_comparator_add (s, c);
      }

    if (i < n1->m_stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n1->m_stages[i]); j++)
      {
        sn_comparator_t *c_orig = SN_STAGE_COMP_GET (n1->m_stages[i], j);
        sn_comparator_t  c_copy;

        SN_COMP_MIN(&c_copy) = SN_COMP_MIN(c_orig) + n0->m_inputs_num;
        SN_COMP_MAX(&c_copy) = SN_COMP_MAX(c_orig) + n0->m_inputs_num;

        sn_stage_comparator_add (s, &c_copy);
      }

    sn_network_stage_add (n, s);
  }

  return (n);
} /* }}} sn_network_t *sn_network_concatenate */

static int sn_network_add_bitonic_merger (sn_network_t *n, /* {{{ */
    int *indizes, int indizes_num)
{
  assert(indizes_num >= 0);
  if (indizes_num <= 1)
    return (0);

  if (indizes_num > 2)
  {
    int const even_indizes_num = (indizes_num + 1) / 2;
    int const odd_indizes_num = indizes_num / 2;

    assert(SIZE_MAX / sizeof(int) >= (size_t) even_indizes_num);
    assert(SIZE_MAX / sizeof(int) >= (size_t) odd_indizes_num);

    {
      int * const even_indizes = (int *) malloc((size_t) even_indizes_num);
      if (!even_indizes)
        return (ENOMEM);
      for (int i = 0; i < even_indizes_num; i++)
        even_indizes[i] = indizes[2 * i];
      int const r = sn_network_add_bitonic_merger(n,
                                                  even_indizes,
                                                  even_indizes_num);
      free(even_indizes);
      if (r)
          return r;
    }
    {
      int * const odd_indizes = (int *) malloc((size_t) odd_indizes_num);
      if (!odd_indizes)
          return (ENOMEM);
      for (int i = 0; i < odd_indizes_num; i++)
        odd_indizes[i] = indizes[(2 * i) + 1];
      int const r = sn_network_add_bitonic_merger(n,
                                                  odd_indizes,
                                                  odd_indizes_num);
      free(odd_indizes);
      if (r)
          return r;
    }
  }

  for (int i = 1; i < indizes_num; i += 2)
  {
    sn_comparator_t c;
    sn_comparator_init(&c, indizes[i - 1], indizes[i]);
    sn_network_comparator_add (n, &c);
  }

  return (0);
} /* }}} int sn_network_add_bitonic_merger */

static int sn_network_add_odd_even_merger (sn_network_t *n, /* {{{ */
    int *indizes_left, int indizes_left_num,
    int *indizes_right, int indizes_right_num)
{
  int tmp_left[indizes_left_num];
  int tmp_left_num;
  int tmp_right[indizes_left_num];
  int tmp_right_num;
  int max_index;
  int i;

  if ((indizes_left_num == 0) || (indizes_right_num == 0))
  {
    return (0);
  }
  else if ((indizes_left_num == 1) && (indizes_right_num == 1))
  {
    sn_comparator_t c;
    sn_stage_t *s;

    sn_comparator_init(&c, *indizes_left, *indizes_right);

    s = sn_stage_create (n->m_stages_num);
    if (s == NULL)
      return (-1);

    sn_stage_comparator_add (s, &c);
    sn_network_stage_add (n, s);

    return (0);
  }

  /* Merge odd sequences */
  tmp_left_num = (indizes_left_num + 1) / 2;
  for (i = 0; i < tmp_left_num; i++)
    tmp_left[i] = indizes_left[2 * i];

  tmp_right_num = (indizes_right_num + 1) / 2;
  for (i = 0; i < tmp_right_num; i++)
    tmp_right[i] = indizes_right[2 * i];

  sn_network_add_odd_even_merger (n,
      tmp_left, tmp_left_num,
      tmp_right, tmp_right_num);

  /* Merge even sequences */
  tmp_left_num = indizes_left_num / 2;
  for (i = 0; i < tmp_left_num; i++)
    tmp_left[i] = indizes_left[(2 * i) + 1];

  tmp_right_num = indizes_right_num / 2;
  for (i = 0; i < tmp_right_num; i++)
    tmp_right[i] = indizes_right[(2 * i) + 1];

  sn_network_add_odd_even_merger (n,
      tmp_left, tmp_left_num,
      tmp_right, tmp_right_num);

  /* Apply ``comparison-interchange'' operations. */
  sn_stage_t * const s = sn_stage_create(n->m_stages_num);

  max_index = indizes_left_num + indizes_right_num;
  if ((max_index % 2) == 0)
    max_index -= 3;
  else
    max_index -= 2;

  for (i = 1; i <= max_index; i += 2)
  {
    sn_comparator_t c;
    sn_comparator_init(
                &c,
                (i < indizes_left_num)
                    ? indizes_left[i]
                    : indizes_right[i - indizes_left_num],
                ((i + 1) < indizes_left_num)
                    ? indizes_left[i + 1]
                    : indizes_right[i + 1 - indizes_left_num]
                );
    sn_stage_comparator_add (s, &c);
  }

  sn_network_stage_add (n, s);

  return (0);
} /* }}} int sn_network_add_odd_even_merger */

sn_network_t *sn_network_combine_bitonic_merge (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  sn_network_t *n0_clone;
  sn_network_t *n;
  int indizes_num = SN_NETWORK_INPUT_NUM (n0) + SN_NETWORK_INPUT_NUM (n1);
  int indizes[indizes_num];
  int i;

  /* We need to invert n0, because the sequence must be
   * z_1 >= z_2 >= ... >= z_k <= z_{k+1} <= ... <= z_p
   * and NOT the other way around! Otherwise the comparators added in
   * sn_network_add_bitonic_merger() from comparing (z_0,z_1), (z_2,z_3), ...
   * to comparing ...,  (z_{n-4},z_{n-3}), (z_{n-2},z_{n-1}), i.e. bound to the
   * end of the list, possibly leaving z_0 uncompared. */
  n0_clone = sn_network_clone (n0);
  if (n0_clone == NULL)
    return (NULL);
  sn_network_invert (n0_clone);

  n = sn_network_concatenate (n0_clone, n1);
  if (n == NULL)
    return (NULL);
  sn_network_destroy (n0_clone);

  for (i = 0; i < indizes_num; i++)
    indizes[i] = i;

  if (!sn_network_add_bitonic_merger (n, indizes, indizes_num))
      return (n);

  sn_network_destroy(n);
  return (NULL);
} /* }}} sn_network_t *sn_network_combine_bitonic_merge */

sn_network_t *sn_network_combine_odd_even_merge (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  sn_network_t *n;
  int indizes_left[n0->m_inputs_num];
  int indizes_left_num;
  int indizes_right[n1->m_inputs_num];
  int indizes_right_num;
  int status;
  int i;

  indizes_left_num = n0->m_inputs_num;
  indizes_right_num = n1->m_inputs_num;
  for (i = 0; i < indizes_left_num; i++)
    indizes_left[i] = i;
  for (i = 0; i < indizes_right_num; i++)
    indizes_right[i] = indizes_left_num + i;

  n = sn_network_concatenate (n0, n1);
  if (n == NULL)
    return (NULL);

  status = sn_network_add_odd_even_merger (n,
      indizes_left, indizes_left_num,
      indizes_right, indizes_right_num);
  if (status != 0)
  {
    sn_network_destroy (n);
    return (NULL);
  }

  sn_network_compress (n);
  return (n);
} /* }}} sn_network_t *sn_network_combine_odd_even_merge */

sn_network_t *sn_network_combine (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  return (sn_network_combine_odd_even_merge (n0, n1));
} /* }}} sn_network_t *sn_network_combine */

int sn_network_sort (sn_network_t *n, int *values) /* {{{ */
{
  int status;
  int i;

  status = 0;
  for (i = 0; i < n->m_stages_num; i++)
  {
    status = sn_stage_sort (n->m_stages[i], values);
    if (status != 0)
      return (status);
  }

  return (status);
} /* }}} int sn_network_sort */

int sn_network_brute_force_check (sn_network_t *n) /* {{{ */
{
  int test_pattern[n->m_inputs_num];
  int values[n->m_inputs_num];
  int status;
  int i;

  for (i = 0; i < n->m_inputs_num; ++i)
      test_pattern[i] = 0;
  while (42)
  {
    int previous;
    int overflow;

    /* Copy the current pattern and let the network sort it */
    memcpy (values, test_pattern, sizeof (values));
    status = sn_network_sort (n, values);
    if (status != 0)
      return (status);

    /* Check if the array is now sorted. */
    previous = values[0];
    for (i = 1; i < n->m_inputs_num; i++)
    {
      if (previous > values[i])
        return (1);
      previous = values[i];
    }

    /* Generate the next test pattern */
    overflow = 1;
    for (i = 0; i < n->m_inputs_num; i++)
    {
      if (test_pattern[i] == 0)
      {
        test_pattern[i] = 1;
        overflow = 0;
        break;
      }
      else
      {
        test_pattern[i] = 0;
        overflow = 1;
      }
    }

    /* Break out of the while loop if we tested all possible patterns */
    if (overflow == 1)
      break;
  } /* while (42) */

  /* All tests successfull */
  return (0);
} /* }}} int sn_network_brute_force_check */

int sn_network_compare (const sn_network_t *n0, const sn_network_t *n1) /* {{{ */
{
  int status;
  int i;

  if (n0 == n1)
    return (0);
  else if (n0 == NULL)
    return (-1);
  else if (n1 == NULL)
    return (1);

  if (n0->m_inputs_num < n1->m_inputs_num)
    return (-1);
  else if (n0->m_inputs_num > n1->m_inputs_num)
    return (1);

  if (n0->m_stages_num < n1->m_stages_num)
    return (-1);
  else if (n0->m_stages_num > n1->m_stages_num)
    return (1);

  for (i = 0; i < n0->m_stages_num; i++)
  {
    status = sn_stage_compare (n0->m_stages[i], n1->m_stages[i]);
    if (status != 0)
      return (status);
  }

  return (0);
} /* }}} int sn_network_compare */
