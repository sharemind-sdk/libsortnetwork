/**
 * libsortnetwork - src/sn-svg.c
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "sn_network.h"

#define INNER_SPACING 15.0
#define OUTER_SPACING 40.0
#define RADIUS 4.0

#define Y_OFFSET 5
#define Y_SPACING 40

static double x_offset = OUTER_SPACING;

static double determine_stage_width (sn_stage_t *s)
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

  return (((double) (lines_used - 1)) * INNER_SPACING);
}

static double determine_network_width (sn_network_t *n) /* {{{ */
{
  double width;
  int i;

  /* Spacing between stages and at the beginning and end of the network */
  width = (SN_NETWORK_STAGE_NUM (n) + 1) * OUTER_SPACING;

  /* Spacing required within a stage */
  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
    width += determine_stage_width (SN_NETWORK_STAGE_GET (n, i));

  return (width);
} /* }}} double determine_network_width */

static int sn_svg_show_stage (sn_stage_t *s)
{
  int lines[s->comparators_num];
  int right[s->comparators_num];
  int lines_used = 0;
  int i;

  printf ("  <!-- stage %i -->\n", SN_STAGE_DEPTH (s));

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

    if (1)
    {
      double x1 = x_offset + (j * INNER_SPACING);
      double x2 = x1;
      int y1 = Y_OFFSET + (SN_COMP_MIN (c) * Y_SPACING);
      int y2 = Y_OFFSET + (SN_COMP_MAX (c) * Y_SPACING);

      printf ("  <line x1=\"%g\" y1=\"%i\" x2=\"%g\" y2=\"%i\" "
	  "stroke=\"black\" stroke-width=\"1\" />\n"
	  "  <circle cx=\"%g\" cy=\"%i\" r=\"%g\" fill=\"black\" />\n"
	  "  <circle cx=\"%g\" cy=\"%i\" r=\"%g\" fill=\"black\" />\n",
	  x1, y1, x2, y2,
	  x1, y1, RADIUS,
	  x2, y2, RADIUS);
    }
  }

  x_offset = x_offset + ((lines_used - 1) * INNER_SPACING) + OUTER_SPACING;

  printf ("\n");

  return (0);
} /* int sn_svg_show_stage */

int main (int argc, char **argv)
{
  sn_network_t *n;
  FILE *fh = NULL;

  double svg_height;
  double svg_width;

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

  svg_height = (2 * Y_OFFSET) + ((SN_NETWORK_INPUT_NUM (n) - 1) * Y_SPACING);
  svg_width = determine_network_width (n);

  printf ("<?xml version=\"1.0\" standalone=\"no\"?>\n"
      "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
      "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
      "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
      "width=\"%gpt\" height=\"%gpt\" viewBox=\"0 0 %g %g\">\n",
      svg_width, svg_height, svg_width, svg_height);

  printf ("<!-- Output generated with sn-svg from %s -->\n", PACKAGE_STRING);

  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
     sn_svg_show_stage (SN_NETWORK_STAGE_GET (n, i));

  printf ("  <!-- horizontal lines -->\n");
  for (i = 0; i < SN_NETWORK_INPUT_NUM (n); i++)
    printf ("  <line x1=\"%g\" y1=\"%i\" x2=\"%g\" y2=\"%i\" "
	"stroke=\"black\" stroke-width=\"1\" />\n",
	0.0, Y_OFFSET + (i * Y_SPACING),
	x_offset, Y_OFFSET + (i * Y_SPACING));

  printf ("</svg>\n");

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
