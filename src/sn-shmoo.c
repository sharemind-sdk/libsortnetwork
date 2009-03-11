/**
 * collectd - src/sn-normalize.c
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
#include <string.h>

#include "sn_network.h"
#include "sn_stage.h"
#include "sn_comparator.h"

struct shmoo_line_info_s
{
  int line_num;
  int zeros_num;
  int ones_num;
};
typedef struct shmoo_line_info_s shmoo_line_info_t;

struct shmoo_chart_s
{
  int inputs_num;
  int stages_num;
  char *state;
};
typedef struct shmoo_chart_s shmoo_chart_t;
#define STATE(sc, i, s, z) \
  (sc)->state[((z) * (sc)->inputs_num * (sc)->stages_num) \
    + ((s) * (sc)->inputs_num) \
    + (i)]

static int account_values (shmoo_chart_t *sc, /* {{{ */
    int stage_num, int zeros_num, int *values)
{
  int i;
  
  for (i = 0; i < sc->inputs_num; i++)
  {
    char state;

    state = STATE (sc, i, stage_num, zeros_num);
    if (values[i] == 0)
    {
      if (state == '-')
        continue;
      else if (state == '1')
        state = '-';
      else /* if ((state == '0') || (state == '?')) */
        state = '0';
    }
    else /* if (values[i] == 1) */
    {
      if (state == '-')
        continue;
      else if (state == '0')
        state = '-';
      else /* if ((state == '1') || (state == '?')) */
        state = '1';
    }

    STATE (sc, i, stage_num, zeros_num) = state;
  }

  return (0);
} /* }}} int account_values */

static int try_all_stages (const sn_network_t *n, /* {{{ */
    shmoo_chart_t *sc, int *pattern)
{
  int values[n->inputs_num];
  int zeros_num;
  int i;

  /* Copy the test pattern */
  memcpy (values, pattern, sizeof (values));

  /* Count the zeros.. */
  zeros_num = 0;
  for (i = 0; i < n->inputs_num; i++)
    if (values[i] == 0)
      zeros_num++;

  for (i = 0; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    
    s = n->stages[i];
    sn_stage_sort (s, values);
    account_values (sc, i, zeros_num, values);
  }

  return (0);
} /* }}} int try_all_stages */

static int try_all_patterns (const sn_network_t *n, /* {{{ */
    shmoo_chart_t *sc)
{
  int test_pattern[n->inputs_num];

  /* Initialize the pattern */
  memset (test_pattern, 0, sizeof (test_pattern));

  while (42)
  {
    int overflow;
    int i;

    try_all_stages (n, sc, test_pattern);
  
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
} /* }}} int try_all_patterns */

static shmoo_chart_t *shmoo_chart_create (const sn_network_t *n)
{
  shmoo_chart_t *sc;
  int state_size;

  sc = malloc (sizeof (*sc));
  memset (sc, 0, sizeof (*sc));

  sc->inputs_num = SN_NETWORK_INPUT_NUM (n);
  sc->stages_num = SN_NETWORK_STAGE_NUM (n);

  state_size = sc->inputs_num * (sc->inputs_num + 1) * sc->stages_num;
  sc->state = calloc (state_size, sizeof (*sc->state));

  memset (sc->state, (int) '?', state_size);

  try_all_patterns (n, sc);

  return (sc);
} /* shmoo_chart_t *shmoo_chart_create */

static int compare_line_info (const void *av, const void *bv) /* {{{ */
{
  const shmoo_line_info_t *al;
  const shmoo_line_info_t *bl;

  al = av;
  bl = bv;

  if (al->zeros_num > bl->zeros_num)
    return (-1);
  else if (al->zeros_num < bl->zeros_num)
    return (1);
  else if (al->ones_num > bl->ones_num)
    return (1);
  else if (al->ones_num < bl->ones_num)
    return (-1);
  else if (al->line_num < bl->line_num)
    return (-1);
  else if (al->line_num > bl->line_num)
    return (1);
  else
    return (0);
} /* }}} int compare_line_info */

/*
 *
 *       11111110000000000
 *       65432109876543210
 *
 *  123: 00000000--------1
 *  111: 000000--------111
 *   23: 0000--------11111
 *  101: 0---------1111111
 */
static void print_shmoo_stage (shmoo_chart_t *sc, int stage_num) /* {{{ */
{
  shmoo_line_info_t line_info[sc->inputs_num];
  char buffer[sc->inputs_num + 2];
  int i;

  for (i = 0; i < sc->inputs_num; i++)
  {
    int j;

    line_info[i].line_num = i;
    line_info[i].zeros_num = 0;
    line_info[i].ones_num = 0;

    for (j = 0; j < sc->inputs_num; j++)
      if (STATE(sc, i, stage_num, j) == '0')
        line_info[i].zeros_num++;
      else if (STATE(sc, i, stage_num, j) == '1')
        line_info[i].ones_num++;
  }

  if (stage_num == -1)
  qsort (line_info, sc->inputs_num, sizeof (line_info[0]),
      compare_line_info);

  printf ("=== After applying stage %i ===\n\n", stage_num);

  if (sc->inputs_num > 9)
  {
    for (i = 0; i < (sc->inputs_num + 1); i++)
    {
      int j;

      j = (sc->inputs_num - i) / 10;

      buffer[i] = '0' + j;
    }
    buffer[sizeof (buffer) - 1] = 0;
    printf ("      %s\n", buffer);
  }

  for (i = 0; i < (sc->inputs_num + 1); i++)
  {
    int j;

    j = (sc->inputs_num - i) % 10;

    buffer[i] = '0' + j;
  }
  buffer[sizeof (buffer) - 1] = 0;
  printf ("      %s\n\n", buffer);

  for (i = 0; i < sc->inputs_num; i++)
  {
    int j;
    int line_num;

    line_num = line_info[i].line_num;

    for (j = 0; j < (sc->inputs_num + 1); j++)
    {
      int zeros_num;

      zeros_num = sc->inputs_num - j;

      buffer[j] = STATE(sc, line_num, stage_num, zeros_num); 
    }
    buffer[sizeof (buffer) - 1] = 0;

    printf (" %3i: %s\n", line_num, buffer);
  }

  printf ("\n\n");
} /* }}} void print_shmoo_stage */

static void print_shmoo (shmoo_chart_t *sc) /* {{{ */
{
  int i;

  for (i = 0; i < sc->stages_num; i++)
    print_shmoo_stage (sc, i);
} /* }}} void print_shmoo */

static void exit_usage (const char *name)
{
  printf ("%s [file]\n", name);
  exit (1);
}

int main (int argc, char **argv)
{
  sn_network_t *n;
  shmoo_chart_t *sc;

  if (argc > 2)
  {
    exit_usage (argv[0]);
  }
  else if (argc == 2)
  {
    if ((strcmp ("-h", argv[1]) == 0)
	|| (strcmp ("--help", argv[1]) == 0)
	|| (strcmp ("-help", argv[1]) == 0))
      exit_usage (argv[0]);

    n = sn_network_read_file (argv[1]);
  }
  else
  {
    n = sn_network_read (stdin);
  }

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sc = shmoo_chart_create (n);
  print_shmoo (sc);

  return (EXIT_SUCCESS);
} /* }}} main */

/* vim: set sw=2 sts=2 et fdm=marker : */
