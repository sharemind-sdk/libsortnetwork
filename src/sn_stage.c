/**
 * collectd - src/sn_stage.c
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
#include <string.h>
#include <assert.h>

#include "sn_comparator.h"
#include "sn_stage.h"

sn_stage_t *sn_stage_create (int depth)
{
  sn_stage_t *s;

  s = (sn_stage_t *) malloc (sizeof (sn_stage_t));
  if (s == NULL)
    return (NULL);
  memset (s, '\0', sizeof (sn_stage_t));

  s->depth = depth;
  
  return (s);
} /* sn_stage_t *sn_stage_create */

void sn_stage_destroy (sn_stage_t *s)
{
  if (s == NULL)
    return;
  if (s->comparators != NULL)
    free (s->comparators);
  free (s);
} /* void sn_stage_destroy */

int sn_stage_sort (sn_stage_t *s, int *values)
{
  sn_comparator_t *c;
  int i;

  for (i = 0; i < s->comparators_num; i++)
  {
    c = s->comparators + i;
    if (values[c->min] > values[c->max])
    {
      int temp;
      
      temp = values[c->min];
      values[c->min] = values[c->max];
      values[c->max] = temp;
    }
  }

  return (0);
} /* int sn_stage_sort */

int sn_stage_comparator_add (sn_stage_t *s, const sn_comparator_t *c)
{
  sn_comparator_t *temp;
  int i;

  temp = (sn_comparator_t *) realloc (s->comparators,
      (s->comparators_num + 1) * sizeof (sn_comparator_t));
  if (temp == NULL)
    return (-1);
  s->comparators = temp;
  temp = NULL;

  for (i = 0; i < s->comparators_num; i++)
    if (sn_comparator_compare (c, s->comparators + i) <= 0)
      break;

  /* Insert the elements in ascending order */
  assert (i <= s->comparators_num);
  if (i < s->comparators_num)
  {
    int nmemb = s->comparators_num - i;
    memmove (s->comparators + (i + 1), s->comparators + i,
	nmemb * sizeof (sn_comparator_t));
  }
  memcpy (s->comparators + i, c, sizeof (sn_comparator_t));
  s->comparators_num++;

  return (0);
} /* int sn_stage_comparator_add */

int sn_stage_comparator_remove (sn_stage_t *s, int c_num)
{
  int nmemb = s->comparators_num - (c_num + 1);
  sn_comparator_t *temp;

  assert (c_num < s->comparators_num);
  assert (c_num >= 0);

  if (nmemb > 0)
    memmove (s->comparators + c_num, s->comparators + (c_num + 1),
	nmemb * sizeof (sn_comparator_t));
  s->comparators_num--;

  /* Free the unused memory */
  if (s->comparators_num == 0)
  {
    free (s->comparators);
    s->comparators = NULL;
  }
  else
  {
    temp = (sn_comparator_t *) realloc (s->comparators,
	s->comparators_num * sizeof (sn_comparator_t));
    if (temp == NULL)
      return (-1);
    s->comparators = temp;
  }

  return (0);
} /* int sn_stage_comparator_remove */

sn_stage_t *sn_stage_clone (const sn_stage_t *s)
{
  sn_stage_t *s_copy;

  s_copy = sn_stage_create (s->depth);
  if (s_copy == NULL)
    return (NULL);

  s_copy->comparators = (sn_comparator_t *) malloc (s->comparators_num
      * sizeof (sn_comparator_t));
  if (s_copy->comparators == NULL)
  {
    free (s_copy);
    return (NULL);
  }

  memcpy (s_copy->comparators, s->comparators,
      s->comparators_num * sizeof (sn_comparator_t));
  s_copy->comparators_num = s->comparators_num;

  return (s_copy);
} /* sn_stage_t *sn_stage_clone */

int sn_stage_comparator_check_conflict (sn_stage_t *s, const sn_comparator_t *c0)
{
  int i;

  for (i = 0; i < s->comparators_num; i++)
  {
    sn_comparator_t *c1 = s->comparators + i;
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

int sn_stage_show (sn_stage_t *s)
{
  int lines[s->comparators_num];
  int right[s->comparators_num];
  int lines_used = 0;
  int i;
  int j;
  int k;

  for (i = 0; i < s->comparators_num; i++)
  {
    lines[i] = -1;
    right[i] = -1;
  }

  for (i = 0; i < s->comparators_num; i++)
  {
    int j;
    sn_comparator_t *c = s->comparators + i;

    for (j = 0; j < lines_used; j++)
      if (SN_COMP_LEFT (c) > right[j])
	break;

    lines[i] = j;
    right[j] = SN_COMP_RIGHT (c);
    if (j >= lines_used)
      lines_used = j + 1;
  }

  for (i = 0; i < lines_used; i++)
  {
    printf ("%3i: ", s->depth);

    for (j = 0; j <= right[i]; j++)
    {
      int on_elem = 0;
      int line_after = 0;

      for (k = 0; k < s->comparators_num; k++)
      {
	sn_comparator_t *c = s->comparators + k;

	/* Check if this comparator is displayed on another line */
	if (lines[k] != i)
	  continue;

	if (j == SN_COMP_MIN (c))
	  on_elem = -1;
	else if (j == SN_COMP_MAX (c))
	  on_elem = 1;

	if ((j >= SN_COMP_LEFT (c)) && (j < SN_COMP_RIGHT (c)))
	  line_after = 1;

	if ((on_elem != 0) || (line_after != 0))
	  break;
      }

      if (on_elem == 0)
      {
	if (line_after == 0)
	  printf ("     ");
	else
	  printf ("-----");
      }
      else if (on_elem == -1)
      {
	if (line_after == 0)
	  printf ("-!   ");
	else
	  printf (" !---");
      }
      else
      {
	if (line_after == 0)
	  printf ("->   ");
	else
	  printf (" <---");
      }
    } /* for (columns) */

    printf ("\n");
  } /* for (lines) */

  return (0);
} /* int sn_stage_show */

int sn_stage_invert (sn_stage_t *s)
{
  int i;

  for (i = 0; i < s->comparators_num; i++)
    sn_comparator_invert (s->comparators + i);

  return (0);
} /* int sn_stage_invert */

int sn_stage_swap (sn_stage_t *s, int con0, int con1)
{
  int i;

  for (i = 0; i < s->comparators_num; i++)
    sn_comparator_swap (s->comparators + i, con0, con1);

  return (0);
} /* int sn_stage_swap */

int sn_stage_cut_at (sn_stage_t *s, int input, enum sn_network_cut_dir_e dir)
{
  int new_position = input;
  int i;

  for (i = 0; i < s->comparators_num; i++)
  {
    sn_comparator_t *c = s->comparators + i;

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

int sn_stage_remove_input (sn_stage_t *s, int input)
{
  int i;

  for (i = 0; i < s->comparators_num; i++)
  {
    sn_comparator_t *c = s->comparators + i;

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

sn_stage_t *sn_stage_read (FILE *fh)
{
  sn_stage_t *s;
  char buffer[64];
  char *buffer_ptr;
  
  s = sn_stage_create (0);
  if (s == NULL)
    return (NULL);

  while (fgets (buffer, sizeof (buffer), fh) != NULL)
  {
    char *endptr;
    sn_comparator_t c;

    if ((buffer[0] == '\0') || (buffer[0] == '\n') || (buffer[0] == '\r'))
      break;
    
    buffer_ptr = buffer;
    endptr = NULL;
    c.min = (int) strtol (buffer_ptr, &endptr, 0);
    if (buffer_ptr == endptr)
      continue;

    buffer_ptr = endptr;
    endptr = NULL;
    c.max = (int) strtol (buffer_ptr, &endptr, 0);
    if (buffer_ptr == endptr)
      continue;

    sn_stage_comparator_add (s, &c);
  }

  if (s->comparators_num == 0)
  {
    sn_stage_destroy (s);
    return (NULL);
  }

  return (s);
} /* sn_stage_t *sn_stage_read */

int sn_stage_write (sn_stage_t *s, FILE *fh)
{
  int i;

  if (s->comparators_num <= 0)
    return (0);

  for (i = 0; i < s->comparators_num; i++)
    fprintf (fh, "%i %i\n",
	SN_COMP_MIN (s->comparators + i),
	SN_COMP_MAX (s->comparators + i));
  fprintf (fh, "\n");

  return (0);
} /* int sn_stage_write */

int sn_stage_serialize (sn_stage_t *s,
    char **ret_buffer, size_t *ret_buffer_size)
{
  char *buffer;
  size_t buffer_size;
  int status;
  int i;

  if (s->comparators_num <= 0)
    return (0);

  buffer = *ret_buffer;
  buffer_size = *ret_buffer_size;

#define SNPRINTF_OR_FAIL(...) \
  status = snprintf (buffer, buffer_size, __VA_ARGS__); \
  if ((status < 1) || (status >= buffer_size)) \
    return (-1); \
  buffer += status; \
  buffer_size -= status;

  for (i = 0; i < s->comparators_num; i++)
  {
    SNPRINTF_OR_FAIL ("%i %i\r\n",
	SN_COMP_MIN (s->comparators + i),
	SN_COMP_MAX (s->comparators + i));
  }

  SNPRINTF_OR_FAIL ("\r\n");

  *ret_buffer = buffer;
  *ret_buffer_size = buffer_size;
  return (0);
} /* int sn_stage_serialize */

sn_stage_t *sn_stage_unserialize (char **ret_buffer, size_t *ret_buffer_size)
{
  sn_stage_t *s;
  char *buffer;
  size_t buffer_size;
  int status;

  buffer = *ret_buffer;
  buffer_size = *ret_buffer_size;

  if (buffer_size == 0)
    return (NULL);

  s = sn_stage_create (0);
  if (s == NULL)
    return (NULL);

  status = 0;
  while (buffer_size > 0)
  {
    sn_comparator_t c;
    char *endptr;
    char *substr;
    size_t substr_length;

    endptr = strchr (buffer, '\n');
    if (endptr == NULL)
    {
      status = -1;
      break;
    }

    *endptr = 0;
    endptr++;

    substr = buffer;
    substr_length = strlen (buffer);
    buffer = endptr;
    buffer_size -= (substr_length + 1);

    if ((substr_length > 0) && (substr[substr_length - 1] == '\r'))
    {
      substr[substr_length - 1] = 0;
      substr_length--;
    }

    if (substr_length == 0)
    {
      status = 0;
      break;
    }

    endptr = NULL;
    c.min = (int) strtol (substr, &endptr, 0);
    if (substr == endptr)
    {
      status = -1;
      break;
    }

    substr = endptr;
    endptr = NULL;
    c.max = (int) strtol (substr, &endptr, 0);
    if (substr == endptr)
    {
      status = -1;
      break;
    }

    sn_stage_comparator_add (s, &c);
  } /* while (buffer_size > 0) */

  if ((status != 0) || (s->comparators_num == 0))
  {
    sn_stage_destroy (s);
    return (NULL);
  }

  *ret_buffer = buffer;
  *ret_buffer_size = buffer_size;
  return (s);
} /* sn_stage_t *sn_stage_unserialize */

/* vim: set shiftwidth=2 softtabstop=2 expandtab : */
