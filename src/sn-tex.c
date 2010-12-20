/**
 * libsortnetwork - src/sn-tex.c
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
#include <unistd.h>
#include <assert.h>

#include "sn_network.h"

/* 21cm (DIN-A4) - 2x3cm Rand = 15cm */
static double output_width = 15.0;
static double scale = 1.0;
static double inner_spacing = 0.3;
static double outer_spacing = 1.0;
static double vertical_spacing = 0.8;

static double x_offset;
static int next_vertex_number = 0;

#define INPUT_TO_Y(i) (((double) (i)) * vertical_spacing)

static void exit_usage (void) /* {{{ */
{
  printf ("Usage: sn-tex [options] [file]\n"
      "\n"
      "Valid options are:\n"
      "  -w <width>   Specify the width of the graph (in TikZ default units).\n" 
      "\n");
  exit (EXIT_FAILURE);
} /* }}} void exit_usage */

static int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "w:h?")) != -1)
  {
    switch (option)
    {
      case 'w':
      {
	double width = atof (optarg);
	if (width <= 0.0)
	{
	  fprintf (stderr, "Invalid width argument: %s\n", optarg);
	  exit (EXIT_FAILURE);
	}
	output_width = width;
	break;
      }

      case 'h':
      case '?':
      default:
	exit_usage ();
    }
  }

  return (0);
} /* }}} int read_options */

static double determine_stage_width (sn_stage_t *s) /* {{{ */
{
  int lines[s->comparators_num];
  int right[s->comparators_num];
  int lines_used = 0;
  int i;

  if (SN_STAGE_COMP_NUM (s) == 0)
    return (0.0);

  for (i = 0; i < SN_STAGE_COMP_NUM (s); i++)
  {
    lines[i] = -1;
    right[i] = -1;
  }

  for (i = 0; i < SN_STAGE_COMP_NUM (s); i++)
  {
    int j;
    sn_comparator_t *c = SN_STAGE_COMP_GET (s, i);

    for (j = 0; j < lines_used; j++)
      if (SN_COMP_LEFT (c) > right[j])
	break;

    lines[i] = j;
    right[j] = SN_COMP_RIGHT (c);
    if (j >= lines_used)
      lines_used = j + 1;
  }
  assert (lines_used >= 1);

  return (((double) (lines_used - 1)) * inner_spacing);
} /* }}} double determine_stage_width */

static double determine_network_width (sn_network_t *n) /* {{{ */
{
  double width;
  int i;

  /* Spacing between stages and at the beginning and end of the network */
  width = (SN_NETWORK_STAGE_NUM (n) + 1) * outer_spacing;

  /* Spacing required within a stage */
  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
    width += determine_stage_width (SN_NETWORK_STAGE_GET (n, i));

  return (width);
} /* }}} double determine_network_width */

static int tex_show_stage (sn_stage_t *s) /* {{{ */
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

    printf ("\\node[vertex] (v%i) at (%.2f,%.2f) {};\n"
	"\\node[vertex] (v%i) at (%.2f,%.2f) {};\n"
	"\\path[comp] (v%i) -- (v%i);\n"
	"\n",
	min_num, x_offset + (j * inner_spacing), INPUT_TO_Y (c->min),
	max_num, x_offset + (j * inner_spacing), INPUT_TO_Y (c->max),
	min_num, max_num);
  }

  x_offset = x_offset + ((lines_used - 1) * inner_spacing) + outer_spacing;

  return (0);
} /* }}} int tex_show_stage */

int main (int argc, char **argv) /* {{{ */
{
  sn_network_t *n;
  FILE *fh = NULL;
  double orig_width;
  int i;

  read_options (argc, argv);

  if ((argc - optind) == 0)
    fh = stdin;
  else if ((argc - optind) == 1)
    fh = fopen (argv[optind], "r");

  if (fh == NULL)
    exit_usage ();

  n = sn_network_read (fh);
  if (n == NULL)
  {
    fprintf (stderr, "Unable to read network from file handle.\n");
    exit (EXIT_FAILURE);
  }

  orig_width = determine_network_width (n);
  if (orig_width <= 0.0)
  {
    fprintf (stderr, "determine_network_width returned invalid value %g.\n",
	orig_width);
    exit (EXIT_FAILURE);
  }

  scale = output_width / orig_width;
  inner_spacing *= scale;
  outer_spacing *= scale;
  vertical_spacing *= scale;

  x_offset = outer_spacing;

  printf ("\\begin{tikzpicture}[auto]\n");

  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
     tex_show_stage (SN_NETWORK_STAGE_GET (n, i));

  for (i = 0; i < SN_NETWORK_INPUT_NUM (n); i++)
    printf ("\\path[edge] (0,%.2f) -- (%.2f,%.2f);\n",
	INPUT_TO_Y (i), x_offset, INPUT_TO_Y (i));

  printf ("\\end{tikzpicture}\n");

  return (0);
} /* }}} int main */

/* vim: set shiftwidth=2 softtabstop=2 fdm=marker : */
