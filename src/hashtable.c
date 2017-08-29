/**
 * \file sn_hashtable.c
 * \brief A hash table for counting the number of sort networks.
 *
 * This is more details about what this file does.
 *
 * \verbatim
 * libsortnetwork - src/sn_hashtable.c
 * Copyright (C) 2011  Florian octo Forster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *   Florian octo Forster <ff at octo.it>
 * \endverbatim
 **/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include "hashtable.h"

struct sn_hashtable_s
{
  uint64_t collisions_num;
  uint64_t networks_num;

  uint8_t ****data;
};

sn_hashtable_t *sn_hashtable_create (void) /* {{{ */
{
  sn_hashtable_t * const ht = (sn_hashtable_t *) malloc(sizeof(*ht));
  if (ht == NULL)
    return (NULL);
  memset (ht, 0, sizeof (*ht));

  ht->data = NULL;

  return (ht);
} /* }}} sn_hashtable_t *sn_hashtable_create */

void sn_hashtable_destroy (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return;

  if (ht->data != NULL)
  {
    int i;

    for (i = 0; i < 65536; i++)
    {
      int j;

      if (ht->data[i] == NULL)
        continue;

      for (j = 0; j < 256; j++)
      {
        int k;

        if (ht->data[i][j] == NULL)
          continue;

        for (k = 0; k < 256; k++)
          free (ht->data[i][j][k]);

        free (ht->data[i][j]);
      }

      free (ht->data[i]);
    }

    free (ht->data);
  }

  free (ht);
} /* }}} void sn_hashtable_destroy */

int sn_hashtable_account (sn_hashtable_t *ht, const sn_network_t *n) /* {{{ */
{
  uint64_t hash;
  uint16_t h0;
  uint8_t h1;
  uint8_t h2;
  uint8_t h3;

  if ((ht == NULL) || (n == NULL))
    return (EINVAL);

  hash = sn_network_get_hashval (n);

  h0 = (uint16_t) (hash >> 24);
  h1 = (uint8_t)  (hash >> 16);
  h2 = (uint8_t)  (hash >>  8);
  h3 = (uint8_t)   hash;

  if (ht->data == NULL)
    ht->data = (uint8_t ****) calloc(65536, sizeof(ht->data[0]));

  if (ht->data[h0] == NULL)
    ht->data[h0] = (uint8_t ***) calloc(256, sizeof(ht->data[h0][0]));

  if (ht->data[h0][h1] == NULL)
    ht->data[h0][h1] = (uint8_t **) calloc(256, sizeof(ht->data[h0][h1][0]));

  if (ht->data[h0][h1][h2] == NULL)
    ht->data[h0][h1][h2] =
            (uint8_t *) calloc(256, sizeof(ht->data[h0][h1][h2][0]));

  assert (sizeof (ht->data[h0][h1][h2][0]) == sizeof (uint8_t));

  if (ht->data[h0][h1][h2][h3] == 0)
    ht->networks_num++;
  else
    ht->collisions_num++;

  if (ht->data[h0][h1][h2][h3] < UINT8_MAX)
    ht->data[h0][h1][h2][h3]++;

  return (0);
} /* }}} int sn_hashtable_account */

_Bool sn_hashtable_check_collision (sn_hashtable_t *ht, const sn_network_t *n) /* {{{ */
{
  uint64_t hash;
  uint16_t h0;
  uint8_t h1;
  uint8_t h2;
  uint8_t h3;

  if ((ht == NULL) || (n == NULL))
    return (0);

  hash = sn_network_get_hashval (n);

  h0 = (uint16_t) (hash >> 24);
  h1 = (uint8_t)  (hash >> 16);
  h2 = (uint8_t)  (hash >>  8);
  h3 = (uint8_t)   hash;

  if (ht->data == NULL)
    return (0);

  if (ht->data[h0] == NULL)
    return (0);

  if (ht->data[h0][h1] == NULL)
    return (0);

  if (ht->data[h0][h1][h2] == NULL)
    return (0);

  assert (sizeof (ht->data[h0][h1][h2][0]) == sizeof (uint8_t));

  if (ht->data[h0][h1][h2][h3] == 0)
    return (0);
  else
    return (1);
} /* }}} _Bool sn_hashtable_check_collision */

uint64_t sn_hashtable_get_collisions (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return (0);

  return (ht->collisions_num);
} /* }}} uint64_t sn_hashtable_get_collisions */

double sn_hashtable_get_collisions_pct (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return (NAN);

  return (100.0 * ((double) ht->collisions_num)
      / ((double) (ht->collisions_num + ht->networks_num)));
} /* }}} double sn_hashtable_get_collisions_pct */

uint64_t sn_hashtable_get_networks (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return (0);

  return (ht->networks_num);
} /* }}} uint64_t sn_hashtable_get_networks */

double sn_hashtable_get_networks_pct (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return (NAN);

  return (100.0 * ((double) ht->networks_num)
      / ((double) (ht->collisions_num + ht->networks_num)));
} /* }}} double sn_hashtable_get_networks_pct */

uint64_t sn_hashtable_get_total (sn_hashtable_t *ht) /* {{{ */
{
  if (ht == NULL)
    return (0);

  return (ht->collisions_num + ht->networks_num);
} /* }}} uint64_t sn_hashtable_get_total */

/* vim: set sw=2 sts=2 et fdm=marker : */
