/**
 * libsortnetwork - src/sn-info.c
 * Copyright (C) 2010  Florian octo Forster
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
#include <strings.h>

#include "sn_network.h"

static _Bool is_normalized (sn_network_t *n) /* {{{ */
{
  int i;

  for (i = 0; i < SN_NETWORK_STAGE_NUM (n); i++)
  {
    sn_stage_t *s = SN_NETWORK_STAGE_GET (n, i);
    int j;

    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c = SN_STAGE_COMP_GET (s, j);

      if (c->min > c->max)
	return (0);
    }
  }

  return (1);
} /* }}} _Bool is_normalized */

int main (int argc, char **argv)
{
  sn_network_t *n;

  int inputs_num;
  int stages_num;
  int comparators_num;
  _Bool normalized;
  _Bool sorts;

  int stages_num_compressed;

  if (argc >= 2)
    n = sn_network_read_file (argv[1]);
  else
    n = sn_network_read (stdin);
  if (n == NULL)
  {
    fprintf (stderr, "Unable to read network.\n");
    exit (EXIT_FAILURE);
  }

  inputs_num = SN_NETWORK_INPUT_NUM (n);
  stages_num = SN_NETWORK_STAGE_NUM (n);
  comparators_num = sn_network_get_comparator_num (n);

  normalized = is_normalized (n);

  if (!normalized)
    sn_network_normalize (n);
  sn_network_compress (n);

  stages_num_compressed = SN_NETWORK_STAGE_NUM (n);

  if (inputs_num <= 16)
  {
    if (sn_network_brute_force_check (n) == 0)
      sorts = 1;
    else
      sorts = 0;
  }
  else
  {
    sorts = 0;
  }

  printf ("%s %s network:\n"
      "\n"
      "  Inputs:      %4i\n",
      (normalized ? "Standard" : "Non-standard"),
      (sorts ? "sorting" : "comparator"),
      inputs_num);

  if (stages_num_compressed == stages_num)
    printf ("  Stages:      %4i\n",
	stages_num);
  else
    printf ("  Stages:      %4i (compressed: %i)\n",
	stages_num, stages_num_compressed);

  printf ("  Comparators: %4i\n"
      "  Normalized:  %4s\n"
      "  Sorts:    %7s\n"
      "  Rating:      %4i\n"
      "  Hash:  0x%08"PRIx32"\n"
      "\n",
      comparators_num,
      (normalized ? "yes" : "no"),
      ((inputs_num > 16) ? "unknown" : (sorts ? "yes" : "no")),
      (stages_num_compressed * inputs_num) + comparators_num,
      sn_network_get_hashval (n));

  exit (EXIT_SUCCESS);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
