/**
 * collectd - src/sn_random.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "sn_random.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int seed;
static int have_init = 0;

static int read_dev_random (void *buffer, size_t buffer_size)
{
  int fd;
  int status = 0;

  char *buffer_position;
  size_t yet_to_read;

  fd = open ("/dev/random", O_RDONLY);
  if (fd < 0)
  {
    perror ("open");
    return (-1);
  }

  buffer_position = (char *) buffer;
  yet_to_read = buffer_size;

  while (yet_to_read > 0)
  {
    status = read (fd, (void *) buffer_position, yet_to_read);
    if (status < 0)
    {
      if (errno == EINTR)
	continue;

      fprintf (stderr, "read_dev_random: read failed.\n");
      break;
    }

    buffer_position += status;
    yet_to_read -= (size_t) status;
  }

  close (fd);

  if (status < 0)
    return (-1);
  return (0);
} /* int read_dev_random */

static void do_init (void)
{
  int status;

  status = read_dev_random (&seed, sizeof (seed));
  if (status == 0)
    have_init = 1;
} /* void do_init */

int sn_random (void)
{
  int ret;

  pthread_mutex_lock (&lock);

  if (have_init == 0)
    do_init ();

  ret = rand_r (&seed);

  pthread_mutex_unlock (&lock);

  return (ret);
} /* int sn_random */

int sn_true_random (void)
{
  int ret = 0;
  int status;

  status = read_dev_random (&ret, sizeof (ret));
  if (status != 0)
    return (sn_random ());

  return (ret);
} /* int sn_true_random */

int sn_bounded_random (int min, int max)
{
  int range;
  int rand;

  if (min == max)
    return (min);
  else if (min > max)
  {
    range = min;
    min = max;
    max = range;
  }

  range = 1 + max - min;
  rand = min + (int) (((double) range)
      * (((double) sn_random ()) / (((double) RAND_MAX) + 1.0)));

  assert (rand >= min);
  assert (rand <= max);

  return (rand);
} /* int sn_bounded_random */

double sn_double_random (void)
{
  return (((double) sn_random ()) / (((double) RAND_MAX) + 1.0));
} /* double sn_double_random */

/* vim: set shiftwidth=2 softtabstop=2 : */
