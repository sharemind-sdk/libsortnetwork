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

#include "config.h"

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "sn_network.h"

const char *input_file = NULL;

/* 21cm (DIN-A4) - 2x3cm Rand = 15cm */
static double output_width = 15.0;
static double scale = 1.0;
static double inner_spacing = 0.3;
static double outer_spacing = 1.0;
static double vertical_spacing = 0.8;

static double x_offset;
static int next_vertex_number = 0;

static int *mask = NULL;

#define INPUT_TO_Y(i) (((double) (i)) * vertical_spacing)

static void exit_usage (void) /* {{{ */
{
  printf ("Usage: sn-tex-cut [options] <num>[:min|max] [<num>[:min|max] ...]\n"
      "\n"
      "Valid options are:\n"
      "  -i <file>    Read comparator network from file.\n"
      "  -w <width>   Specify the width of the graph (in TikZ default units).\n" 
      "\n");
  exit (EXIT_FAILURE);
} /* }}} void exit_usage */

static int read_options (int argc, char **argv) /* {{{ */
{
  int option;

  while ((option = getopt (argc, argv, "w:i:h?")) != -1)
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

      case 'i':
      input_file = optarg;
      break;

      case 'h':
      case '?':
      default:
	exit_usage ();
    }
  }

  return (0);
} /* }}} int read_options */

static int read_mask (sn_network_t *n, int argc, char **argv) /* {{{ */
{
  int i;

  mask = calloc (SN_NETWORK_INPUT_NUM (n), sizeof (*mask));
  if (mask == NULL)
    return (ENOMEM);

  for (i = 0; i < argc; i++)
  {
    char *endptr = NULL;
    int pos;
    int val = 1;

    pos = (int) strtol (argv[i], &endptr, /* base = */ 0);
    if (strcasecmp (":min", endptr) == 0)
      val = -1;

    if ((pos < 0) || (pos >= SN_NETWORK_INPUT_NUM (n)))
    {
      fprintf (stderr, "Invalid line number: %i\n", pos);
      return (-1);
    }

    mask[pos] = val;
  }

  return (0);
} /* }}} int read_mask */

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

static int stage_print_lines (sn_stage_t *s, int *mask, double *x_left) /* {{{ */
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
    double x_right;
    int comp_left;
    int comp_right;
    int mask_left;
    int mask_right;
    const char *left_class  = "edge";
    const char *right_class = "edge";

    comp_left  = SN_COMP_LEFT (c);
    comp_right = SN_COMP_RIGHT (c);

    mask_left = mask[comp_left];
    mask_right = mask[comp_right];

    for (j = 0; j < lines_used; j++)
      if (comp_left > right[j])
	break;

    lines[i] = j;
    right[j] = comp_right;
    if (j >= lines_used)
      lines_used = j + 1;

    if ((mask_left == mask_right)
	|| ((mask_left <= 0) && (mask_right >= 0)))
      continue;

    if (mask_left > 0)
      left_class = "edge maximum";
    if (mask_right < 0)
      right_class = "edge minimum";

    x_right = x_offset + (j * inner_spacing);

    printf ("\\path[%s] (%.2f,%.2f) -- (%.2f,%.2f);\n",
	left_class,
	x_left[comp_left], INPUT_TO_Y (comp_left),
	x_right, INPUT_TO_Y (comp_left));
    printf ("\\path[%s] (%.2f,%.2f) -- (%.2f,%.2f);\n",
	right_class,
	x_left[comp_right], INPUT_TO_Y (comp_right),
	x_right, INPUT_TO_Y (comp_right));

    x_left[comp_left] = x_right;
    x_left[comp_right] = x_right;
  }

  x_offset = x_offset + ((lines_used - 1) * inner_spacing) + outer_spacing;

  return (0);
} /* }}} int stage_print_lines */

static int network_print_lines (sn_network_t *n, int *mask_orig) /* {{{ */
{
  int mask[SN_NETWORK_INPUT_NUM (n)];
  double x_left[SN_NETWORK_INPUT_NUM (n)];
  int i;

  memcpy (mask, mask_orig, sizeof (mask));
  for (i = 0; i < SN_NETWORK_INPUT_NUM (n); i++)
    x_left[i] = 0.0;

  x_offset = outer_spacing;
  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
  {
    stage_print_lines (SN_NETWORK_STAGE_GET (n, i), mask, x_left);
    sn_stage_sort (SN_NETWORK_STAGE_GET (n, i), mask);
  }

  for (i = 0; i < SN_NETWORK_INPUT_NUM (n); i++)
  {
    const char *class = "edge";

    if (mask[i] < 0)
      class = "edge minimum";
    else if (mask[i] > 0)
      class = "edge maximum";

    printf ("\\path[%s] (%.2f,%.2f) -- (%.2f,%.2f);\n",
	class,
	x_left[i], INPUT_TO_Y (i),
	x_offset, INPUT_TO_Y (i));
  }

  return (0);
} /* }}} int network_print_lines */

static const char *mask_to_class (int left, int right, int *mask) /* {{{ */
{
  if (mask[left] == mask[right])
  {
    if (mask[left] == 0)
      return ("");
    else if (mask[left] > 0)
      return (" inactive maximum");
    else
      return (" inactive minimum");
  }
  else if (mask[left] < mask[right])
  {
    if ((mask[left] < 0) && (mask[right] > 0))
      return (" inactive minimum maximum");
    else if (mask[left] < 0)
      return (" inactive minimum");
    else
      return (" inactive maximum");
  }
  else
  {
    const char *class;
    int tmp;

    if ((mask[left] > 0) && (mask[right] < 0))
      class = " active minimum maximum";
    else if (mask[left] > 0)
      class = " active maximum";
    else
      class = " active minimum";

    tmp = mask[left];
    mask[left] = mask[right];
    mask[right] = tmp;

    return (class);
  }
} /* }}} const char *mask_to_class */

static int stage_print_comparators (sn_stage_t *s, int *mask) /* {{{ */
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
    const char *class;

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

    class = mask_to_class (SN_COMP_LEFT (c), SN_COMP_RIGHT (c), mask);

    printf ("\\node[vertex%s] (v%i) at (%.2f,%.2f) {};\n"
	"\\node[vertex%s] (v%i) at (%.2f,%.2f) {};\n"
	"\\path[comp%s] (v%i) -- (v%i);\n"
	"\n",
	class, min_num, x_offset + (j * inner_spacing), INPUT_TO_Y (c->min),
	class, max_num, x_offset + (j * inner_spacing), INPUT_TO_Y (c->max),
	class, min_num, max_num);
  }

  x_offset = x_offset + ((lines_used - 1) * inner_spacing) + outer_spacing;

  return (0);
} /* }}} int stage_print_comparators */

static int network_print_comparators (sn_network_t *n, int *mask_orig) /* {{{ */
{
  int mask[SN_NETWORK_INPUT_NUM (n)];
  int i;

  memcpy (mask, mask_orig, sizeof (mask));

  x_offset = outer_spacing;
  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
    stage_print_comparators (SN_NETWORK_STAGE_GET (n, i), mask);

  return (0);
} /* }}} int network_print_comparators */

int main (int argc, char **argv) /* {{{ */
{
  sn_network_t *n;
  FILE *fh = NULL;
  double orig_width;
  int status;

  read_options (argc, argv);

  if (input_file == NULL)
    fh = stdin;
  else
    fh = fopen (input_file, "r");

  if (fh == NULL)
    exit_usage ();

  n = sn_network_read (fh);
  if (n == NULL)
  {
    fprintf (stderr, "Unable to read network from file handle.\n");
    exit (EXIT_FAILURE);
  }

  status = read_mask (n, argc - optind, argv + optind);
  if (status != 0)
  {
    fprintf (stderr, "read_mask failed.\n");
    exit (EXIT_FAILURE);
  }
  assert (mask != NULL);
    
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

  printf ("\\begin{tikzpicture}[auto]\n");

  network_print_lines (n, mask);
  network_print_comparators (n, mask);

  printf ("\\end{tikzpicture}\n");

  return (0);
} /* }}} int main */

/* vim: set shiftwidth=2 softtabstop=2 fdm=marker : */
