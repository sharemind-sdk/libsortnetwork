/**
 * collectd - src/sn_network.c
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

#if 0
# define DPRINTF(...) fprintf (stderr, "sn_network: " __VA_ARGS__)
#else
# define DPRINTF(...) /**/
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>

#include "sn_network.h"
#include "sn_random.h"

sn_network_t *sn_network_create (int inputs_num) /* {{{ */
{
  sn_network_t *n;

  n = (sn_network_t *) malloc (sizeof (sn_network_t));
  if (n == NULL)
    return (NULL);
  memset (n, '\0', sizeof (sn_network_t));

  n->inputs_num = inputs_num;

  return (n);
} /* }}} sn_network_t *sn_network_create */

void sn_network_destroy (sn_network_t *n) /* {{{ */
{
  if (n == NULL)
    return;

  if (n->stages != NULL)
  {
    int i;
    for (i = 0; i < n->stages_num; i++)
    {
      sn_stage_destroy (n->stages[i]);
      n->stages[i] = NULL;
    }
    free (n->stages);
    n->stages = NULL;
  }

  free (n);
} /* }}} void sn_network_destroy */

int sn_network_stage_add (sn_network_t *n, sn_stage_t *s) /* {{{ */
{
  sn_stage_t **temp;

  temp = (sn_stage_t **) realloc (n->stages, (n->stages_num + 1)
      * sizeof (sn_stage_t *));
  if (temp == NULL)
    return (-1);

  n->stages = temp;
  SN_STAGE_DEPTH (s) = n->stages_num;
  n->stages[n->stages_num] = s;
  n->stages_num++;

  return (0);
} /* }}} int sn_network_stage_add */

int sn_network_stage_remove (sn_network_t *n, int s_num) /* {{{ */
{
  int nmemb = n->stages_num - (s_num + 1);
  sn_stage_t **temp;

  assert (s_num < n->stages_num);

  sn_stage_destroy (n->stages[s_num]);
  n->stages[s_num] = NULL;

  if (nmemb > 0)
  {
    memmove (n->stages + s_num, n->stages + (s_num + 1),
	nmemb * sizeof (sn_stage_t *));
    n->stages[n->stages_num - 1] = NULL;
  }
  n->stages_num--;

  /* Free the unused memory */
  if (n->stages_num == 0)
  {
    free (n->stages);
    n->stages = NULL;
  }
  else
  {
    temp = (sn_stage_t **) realloc (n->stages,
	n->stages_num * sizeof (sn_stage_t *));
    if (temp == NULL)
      return (-1);
    n->stages = temp;
  }

  return (0);
} /* }}} int sn_network_stage_remove */

sn_network_t *sn_network_clone (const sn_network_t *n) /* {{{ */
{
  sn_network_t *n_copy;
  int i;

  n_copy = sn_network_create (n->inputs_num);
  if (n_copy == NULL)
    return (NULL);

  for (i = 0; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    int status;

    s = sn_stage_clone (n->stages[i]);
    if (s == NULL)
      break;

    status = sn_network_stage_add (n_copy, s);
    if (status != 0)
      break;
  }

  if (i < n->stages_num)
  {
    sn_network_destroy (n_copy);
    return (NULL);
  }

  return (n_copy);
} /* }}} sn_network_t *sn_network_clone */

int sn_network_show (sn_network_t *n) /* {{{ */
{
  int i;

  for (i = 0; i < n->stages_num; i++)
    sn_stage_show (n->stages[i]);

  return (0);
} /* }}} int sn_network_show */

int sn_network_invert (sn_network_t *n) /* {{{ */
{
  int i;

  for (i = 0; i < n->stages_num; i++)
    sn_stage_invert (n->stages[i]);

  return (0);
} /* }}} int sn_network_invert */

int sn_network_shift (sn_network_t *n, int sw) /* {{{ */
{
  int i;

  for (i = 0; i < n->stages_num; i++)
    sn_stage_shift (n->stages[i], sw, SN_NETWORK_INPUT_NUM (n));

  return (0);
} /* }}} int sn_network_shift */

int sn_network_compress (sn_network_t *n) /* {{{ */
{
  int i;
  int j;
  int k;

  for (i = 1; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    
    s = n->stages[i];
    
    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c = SN_STAGE_COMP_GET (s, j);
      int move_to = i;

      for (k = i - 1; k >= 0; k--)
      {
	int conflict;

	conflict = sn_stage_comparator_check_conflict (n->stages[k], c);
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
	  sn_stage_comparator_add (n->stages[move_to], c);
	sn_stage_comparator_remove (s, j);
	j--;
      }
    }
  }

  while ((n->stages_num > 0)
      && (SN_STAGE_COMP_NUM (n->stages[n->stages_num - 1]) == 0))
    sn_network_stage_remove (n, n->stages_num - 1);

  return (0);
} /* }}} int sn_network_compress */

int sn_network_normalize (sn_network_t *n) /* {{{ */
{
  int i;

  for (i = n->stages_num - 1; i >= 0; i--)
  {
    sn_stage_t *s;
    int j;

    s = n->stages[i];

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c;
      int min;
      int max;

      c = SN_STAGE_COMP_GET (s, j);

      min = c->min;
      max = c->max;

      if (min > max)
      {
	int k;

	for (k = i; k >= 0; k--)
	  sn_stage_swap (n->stages[k], min, max);
      }
    } /* for (j = 0 .. #comparators) */
  } /* for (i = n->stages_num - 1 .. 0) */

  return (0);
} /* }}} int sn_network_normalize */

int sn_network_cut_at (sn_network_t *n, int input, /* {{{ */
    enum sn_network_cut_dir_e dir)
{
  int i;
  int position = input;

  for (i = 0; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    int new_position;
    
    s = n->stages[i];
    new_position = sn_stage_cut_at (s, position, dir);
    
    if (position != new_position)
    {
      int j;

      for (j = 0; j < i; j++)
	sn_stage_swap (n->stages[j], position, new_position);
    }

    position = new_position;
  }

  assert (((dir == DIR_MIN) && (position == 0))
      || ((dir == DIR_MAX) && (position == (n->inputs_num - 1))));

  for (i = 0; i < n->stages_num; i++)
    sn_stage_remove_input (n->stages[i], position);

  n->inputs_num--;

  return (0);
} /* }}} int sn_network_cut_at */

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

  stages_num = (n0->stages_num > n1->stages_num)
    ? n0->stages_num
    : n1->stages_num;

  n = sn_network_create (n0->inputs_num + n1->inputs_num);
  if (n == NULL)
    return (NULL);

  for (i = 0; i < stages_num; i++)
  {
    sn_stage_t *s = sn_stage_create (i);

    if (i < n0->stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n0->stages[i]); j++)
      {
	sn_comparator_t *c = SN_STAGE_COMP_GET (n0->stages[i], j);
	sn_stage_comparator_add (s, c);
      }

    if (i < n1->stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n1->stages[i]); j++)
      {
	sn_comparator_t *c_orig = SN_STAGE_COMP_GET (n1->stages[i], j);
	sn_comparator_t  c_copy;

	SN_COMP_MIN(&c_copy) = SN_COMP_MIN(c_orig) + n0->inputs_num;
	SN_COMP_MAX(&c_copy) = SN_COMP_MAX(c_orig) + n0->inputs_num;

	sn_stage_comparator_add (s, &c_copy);
      }

    sn_network_stage_add (n, s);
  }

  return (n);
} /* }}} sn_network_t *sn_network_concatenate */

static int sn_network_add_bitonic_merger_recursive (sn_network_t *n, /* {{{ */
    int low, int num)
{
  sn_stage_t *s;
  int m;
  int i;

  if (num == 1)
    return (0);

  s = sn_stage_create (n->stages_num);
  if (s == NULL)
    return (-1);

  m = num / 2;

  for (i = low; i < (low + m); i++)
  {
    sn_comparator_t c;

    c.min = i;
    c.max = i + m;

    sn_stage_comparator_add (s, &c);
  }

  sn_network_stage_add (n, s);

  sn_network_add_bitonic_merger_recursive (n, low, m);
  sn_network_add_bitonic_merger_recursive (n, low + m, m);

  return (0);
} /* }}} int sn_network_add_bitonic_merger_recursive */

static int sn_network_add_bitonic_merger (sn_network_t *n) /* {{{ */
{
#if 0
  sn_stage_t *s;
  int m;
  int i;

  s = sn_stage_create (n->stages_num);
  if (s == NULL)
    return (-1);

  m = n->inputs_num / 2;

  for (i = 0; i < m; i++)
  {
    sn_comparator_t c;

    c.min = i;
    c.max = n->inputs_num - (i + 1);

    sn_stage_comparator_add (s, &c);
  }

  sn_network_stage_add (n, s);

  sn_network_add_bitonic_merger_recursive (n, 0, m);
  sn_network_add_bitonic_merger_recursive (n, m, m);
#else
  sn_network_add_bitonic_merger_recursive (n, 0, SN_NETWORK_INPUT_NUM (n));
#endif

  return (0);
} /* }}} int sn_network_add_bitonic_merger */

static int sn_network_add_odd_even_merger_recursive (sn_network_t *n, /* {{{ */
    int *indizes, int indizes_num)
{
  if (indizes_num > 2)
  {
    sn_comparator_t c;
    sn_stage_t *s;
    int indizes_half_num;
    int *indizes_half;
    int status;
    int i;

    indizes_half_num = indizes_num / 2;
    indizes_half = (int *) malloc (indizes_num * sizeof (int));
    if (indizes_half == NULL)
      return (-1);

    for (i = 0; i < indizes_half_num; i++)
    {
      indizes_half[i] = indizes[2 * i];
      indizes_half[indizes_half_num + i] = indizes[(2 * i) + 1];
    }

    status = sn_network_add_odd_even_merger_recursive (n,
	indizes_half, indizes_half_num);
    if (status != 0)
    {
      free (indizes_half);
      return (status);
    }

    status = sn_network_add_odd_even_merger_recursive (n,
	indizes_half + indizes_half_num, indizes_half_num);
    if (status != 0)
    {
      free (indizes_half);
      return (status);
    }

    free (indizes_half);

    s = sn_stage_create (n->stages_num);
    if (s == NULL)
      return (-1);

    for (i = 1; i < (indizes_num - 2); i += 2)
    {
      c.min = indizes[i];
      c.max = indizes[i + 1];

      sn_stage_comparator_add (s, &c);
    }

    sn_network_stage_add (n, s);
  }
  else
  {
    sn_comparator_t c;
    sn_stage_t *s;

    assert (indizes_num == 2);

    c.min = indizes[0];
    c.max = indizes[1];

    s = sn_stage_create (n->stages_num);
    if (s == NULL)
      return (-1);

    sn_stage_comparator_add (s, &c);
    sn_network_stage_add (n, s);
  }

  return (0);
} /* }}} int sn_network_add_odd_even_merger_recursive */

static int sn_network_add_odd_even_merger (sn_network_t *n) /* {{{ */
{
  int *indizes;
  int indizes_num;
  int status;
  int i;

  indizes_num = n->inputs_num;
  indizes = (int *) malloc (indizes_num * sizeof (int));
  if (indizes == NULL)
    return (-1);

  for (i = 0; i < indizes_num; i++)
    indizes[i] = i;

  status = sn_network_add_odd_even_merger_recursive (n,
      indizes, indizes_num);
  
  free (indizes);
  return (status);
} /* }}} int sn_network_add_bitonic_merger */

static sn_network_t *sn_network_combine_bitonic (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  sn_network_t *n;
  sn_network_t *n1_clone;
  int shift;

  n1_clone = sn_network_clone (n1);
  if (n1_clone == NULL)
    return (NULL);

  sn_network_invert (n1_clone);

  n = sn_network_concatenate (n0, n1_clone);
  if (n == NULL)
    return (NULL);

  sn_network_destroy (n1_clone);

  shift = sn_bounded_random (0, SN_NETWORK_INPUT_NUM (n) - 1);
  if (shift > 0)
  {
    DPRINTF ("sn_network_combine_bitonic: Shifting by %i.\n", shift);
    sn_network_shift (n, shift);
  }

  sn_network_add_bitonic_merger (n);

  return (n);
} /* }}} sn_network_t *sn_network_combine_bitonic */

sn_network_t *sn_network_combine (sn_network_t *n0, /* {{{ */
    sn_network_t *n1)
{
  sn_network_t *n;

  if (sn_bounded_random (0, 2) < 2)
  {
    DPRINTF ("sn_network_combine: Using the bitonic merger.\n");
    n = sn_network_combine_bitonic (n0, n1);
  }
  else
  {
    DPRINTF ("sn_network_combine: Using the odd-even merger.\n");
    n = sn_network_concatenate (n0, n1);
    if (n == NULL)
      return (NULL);
    sn_network_add_odd_even_merger (n);
  }

  sn_network_compress (n);

  return (n);
} /* }}} sn_network_t *sn_network_combine */

int sn_network_sort (sn_network_t *n, int *values) /* {{{ */
{
  int status;
  int i;

  status = 0;
  for (i = 0; i < n->stages_num; i++)
  {
    status = sn_stage_sort (n->stages[i], values);
    if (status != 0)
      return (status);
  }

  return (status);
} /* }}} int sn_network_sort */

int sn_network_brute_force_check (sn_network_t *n) /* {{{ */
{
  int test_pattern[n->inputs_num];
  int values[n->inputs_num];
  int status;
  int i;

  memset (test_pattern, 0, sizeof (test_pattern));
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
    for (i = 1; i < n->inputs_num; i++)
    {
      if (previous > values[i])
	return (1);
      previous = values[i];
    }

    /* Generate the next test pattern */
    overflow = 1;
    for (i = 0; i < n->inputs_num; i++)
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

sn_network_t *sn_network_read (FILE *fh) /* {{{ */
{
  sn_network_t *n;
  char buffer[64];

  int opt_inputs = 0;

  while (fgets (buffer, sizeof (buffer), fh) != NULL)
  {
    char *str_key = buffer;
    char *str_value = NULL;
    int   buffer_len = strlen (buffer);

    while ((buffer_len > 0) && ((buffer[buffer_len - 1] == '\n')
	  || (buffer[buffer_len - 1] == '\r')))
    {
      buffer_len--;
      buffer[buffer_len] = '\0';
    }
    if (buffer_len == 0)
      break;

    str_value = strchr (buffer, ':');
    if (str_value == NULL)
    {
      printf ("Cannot parse line: %s\n", buffer);
      continue;
    }

    *str_value = '\0'; str_value++;
    while ((*str_value != '\0') && (isspace (*str_value) != 0))
      str_value++;

    if (strcasecmp ("Inputs", str_key) == 0)
      opt_inputs = atoi (str_value);
    else
      printf ("Unknown key: %s\n", str_key);
  } /* while (fgets) */

  if (opt_inputs < 2)
    return (NULL);

  n = sn_network_create (opt_inputs);

  while (42)
  {
    sn_stage_t *s;

    s = sn_stage_read (fh);
    if (s == NULL)
      break;

    sn_network_stage_add (n, s);
  }

  if (SN_NETWORK_STAGE_NUM (n) < 1)
  {
    sn_network_destroy (n);
    return (NULL);
  }

  return (n);
} /* }}} sn_network_t *sn_network_read */

sn_network_t *sn_network_read_file (const char *file) /* {{{ */
{
  sn_network_t *n;
  FILE *fh;

  fh = fopen (file, "r");
  if (fh == NULL)
    return (NULL);

  n = sn_network_read (fh);

  fclose (fh);

  return (n);
} /* }}} sn_network_t *sn_network_read_file */

int sn_network_write (sn_network_t *n, FILE *fh) /* {{{ */
{
  int i;

  fprintf (fh, "Inputs: %i\n", n->inputs_num);
  fprintf (fh, "\n");

  for (i = 0; i < n->stages_num; i++)
    sn_stage_write (n->stages[i], fh);

  return (0);
} /* }}} int sn_network_write */

int sn_network_write_file (sn_network_t *n, const char *file) /* {{{ */
{
  int status;
  FILE *fh;

  fh = fopen (file, "w");
  if (fh == NULL)
    return (-1);

  status = sn_network_write (n, fh);

  fclose (fh);

  return (status);
} /* }}} int sn_network_write_file */

int sn_network_serialize (sn_network_t *n, char **ret_buffer, /* {{{ */
    size_t *ret_buffer_size)
{
  char *buffer;
  size_t buffer_size;
  int status;
  int i;

  buffer = *ret_buffer;
  buffer_size = *ret_buffer_size;

#define SNPRINTF_OR_FAIL(...) \
  status = snprintf (buffer, buffer_size, __VA_ARGS__); \
  if ((status < 1) || (status >= buffer_size)) \
    return (-1); \
  buffer += status; \
  buffer_size -= status;

  SNPRINTF_OR_FAIL ("Inputs: %i\r\n\r\n", n->inputs_num);

  for (i = 0; i < n->stages_num; i++)
  {
    status = sn_stage_serialize (n->stages[i], &buffer, &buffer_size);
    if (status != 0)
      return (status);
  }

  *ret_buffer = buffer;
  *ret_buffer_size = buffer_size;
  return (0);
} /* }}} int sn_network_serialize */

sn_network_t *sn_network_unserialize (char *buffer, /* {{{ */
    size_t buffer_size)
{
  sn_network_t *n;
  int opt_inputs = 0;

  if (buffer_size == 0)
    return (NULL);

  /* Read options first */
  while (buffer_size > 0)
  {
    char *endptr;
    char *str_key;
    char *str_value;
    char *line;
    int   line_len;

    line = buffer;
    endptr = strchr (buffer, '\n');
    if (endptr == NULL)
      return (NULL);

    *endptr = 0;
    endptr++;
    buffer = endptr;
    line_len = strlen (line);

    if ((line_len > 0) && (line[line_len - 1] == '\r'))
    {
      line[line_len - 1] = 0;
      line_len--;
    }

    if (line_len == 0)
      break;

    str_key = line;
    str_value = strchr (line, ':');
    if (str_value == NULL)
    {
      printf ("Cannot parse line: %s\n", line);
      continue;
    }

    *str_value = '\0'; str_value++;
    while ((*str_value != '\0') && (isspace (*str_value) != 0))
      str_value++;

    if (strcasecmp ("Inputs", str_key) == 0)
      opt_inputs = atoi (str_value);
    else
      printf ("Unknown key: %s\n", str_key);
  } /* while (fgets) */

  if (opt_inputs < 2)
    return (NULL);

  n = sn_network_create (opt_inputs);

  while (42)
  {
    sn_stage_t *s;

    s = sn_stage_unserialize (&buffer, &buffer_size);
    if (s == NULL)
      break;

    sn_network_stage_add (n, s);
  }

  if (SN_NETWORK_STAGE_NUM (n) < 1)
  {
    sn_network_destroy (n);
    return (NULL);
  }

  return (n);
} /* }}} sn_network_t *sn_network_unserialize */

/* vim: set sw=2 sts=2 et fdm=marker : */
