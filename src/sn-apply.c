/**
 * collectd - src/sn-apply.c
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>

#include "sn_network.h"
#include "sn_population.h"
#include "sn_random.h"

static sn_network_t *network = NULL;

static void exit_usage (const char *name)
{
  printf ("Usage: %s [options]\n"
      "\n"
      "Valid options are:\n"
      "  -i <file>     File holding the network (REQUIRED)\n"
      "\n",
      name);
  exit (1);
} /* void exit_usage */

int read_options (int argc, char **argv)
{
  int option;

  while ((option = getopt (argc, argv, "i:")) != -1)
  {
    switch (option)
    {
      case 'i':
      {
	if (network != NULL)
	  sn_network_destroy (network);
	network = sn_network_read_file (optarg);
	break;
      }

      case 'h':
      default:
	exit_usage (argv[0]);
    }
  }

  if (network == NULL)
  {
    fprintf (stderr, "No network (`-i' option) was given or failed to read.\n");
    return (-1);
  }

  return (0);
} /* int read_options */

static int read_value (int *ret_value)
{
  char buffer[4096];
  char *bufptr;
  char *endptr;

  bufptr = fgets (buffer, sizeof (buffer), stdin);
  if (bufptr == NULL)
  {
    if (feof (stdin))
      return (1);
    else
    {
      fprintf (stderr, "fgets failed.\n");
      return (-1);
    }
  }

  endptr = NULL;
  *ret_value = strtol (bufptr, &endptr, 0);
  if (endptr == bufptr)
  {
    fprintf (stderr, "strtol failed.\n");
    return (-1);
  }

  return (0);
} /* int read_value */

static int show_values (int values_num, const int *values)
{
  int i;

  assert (values_num > 0);

  fprintf (stdout, "%i", values[0]);
  for (i = 1; i < values_num; i++)
    fprintf (stdout, " %i", values[i]);
  fprintf (stdout, "\n");
  fflush (stdout);

  return (0);
} /* int show_values */

static int show_sort (int *values)
{
  int stages_num;
  int i;

  stages_num = SN_NETWORK_STAGE_NUM (network);

  show_values (SN_NETWORK_INPUT_NUM (network), values);
  for (i = 0; i < stages_num; i++)
  {
    sn_stage_t *s;

    s = SN_NETWORK_STAGE_GET (network, i);
    sn_stage_sort (s, values);

    show_values (SN_NETWORK_INPUT_NUM (network), values);
  } /* for (stages) */

  return (0);
} /* int show_sort */

static int read_values (int values_num, int *ret_values)
{
  int status;
  int i;

  status = 0;
  for (i = 0; i < values_num; i++)
  {
    status = read_value (ret_values + i);
    if (status != 0)
      break;
  }

  return (status);
} /* int read_values */

int main (int argc, char **argv)
{
  int inputs_num;
  int *values;
  int status;

  status = read_options (argc, argv);
  if (status != 0)
    return (1);

  inputs_num = SN_NETWORK_INPUT_NUM (network);

  values = (int *) malloc (inputs_num * sizeof (int));
  if (values == NULL)
  {
    fprintf (stderr, "malloc failed.\n");
    return (1);
  }

  while (42)
  {
    status = read_values (inputs_num, values);
    if (status != 0)
      break;

    show_sort (values);
  }

  if (status < 0)
    return (1);
  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
