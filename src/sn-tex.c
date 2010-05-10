/**
 * collectd - src/sn-tex.c
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
#include <stdio.h>

#define SCALE 0.7
#define INNER_SPACING 0.35
#define OUTER_SPACING 1.5

#include "sn_network.h"

static double x_offset = OUTER_SPACING;
static int next_vertex_number = 0;

static int tex_show_stage (sn_stage_t *s)
{
  int lines[s->comparators_num];
  int right[s->comparators_num];
  int lines_used = 0;
  int i;

  for (i = 0; i < s->comparators_num; i++)
  {
    lines[i] = -1;
    right[i] = -1;
  }

  for (i = 0; i < s->comparators_num; i++)
  {
    int j;
    sn_comparator_t *c = s->comparators + i;

    int min_num;
    int max_num;

    min_num = next_vertex_number;
    next_vertex_number++;

    max_num = next_vertex_number;
    next_vertex_number++;

    for (j = 0; j < lines_used; j++)
      if (SN_COMP_LEFT (c) > right[j])
	break;

    lines[i] = j;
    right[j] = SN_COMP_RIGHT (c);
    if (j >= lines_used)
      lines_used = j + 1;

    printf ("\\node[vertex] (v%i) at (%.2f,%i) {};\n"
	"\\node[vertex] (v%i) at (%.2f,%i) {};\n"
	"\\path[comp] (v%i) -- (v%i);\n"
	"\n",
	min_num, x_offset + (j * INNER_SPACING), c->min,
	max_num, x_offset + (j * INNER_SPACING), c->max,
	min_num, max_num);
  }

  x_offset = x_offset + ((lines_used - 1) * INNER_SPACING) + OUTER_SPACING;

  return (0);
} /* int tex_show_stage */

int main (int argc, char **argv)
{
  sn_network_t *n;
  FILE *fh = NULL;
  int i;

  if (argc == 1)
    fh = stdin;
  else if (argc == 2)
    fh = fopen (argv[1], "r");

  if (fh == NULL)
  {
    printf ("fh == NULL!\n");
    return (1);
  }

  n = sn_network_read (fh);

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  printf ("\\begin{tikzpicture}[scale=%.2f,auto]\n", SCALE);

  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
     tex_show_stage (SN_NETWORK_STAGE_GET (n, i));

  for (i = 0; i < SN_NETWORK_INPUT_NUM (n); i++)
    printf ("\\path[edge] (0,%i) -- (%.2f,%i);\n",
	i, x_offset, i);

  printf ("\\end{tikzpicture}\n");

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
