/**
 * \file sn_hashtable.h
 * \brief A hash table for counting the number of sort networks.
 *
 * This is more details about what this file does.
 *
 * \verbatim
 * libsortnetwork - src/sn_hashtable.h
 * Copyright (C) 2011  Florian octo Forster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
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

#ifndef SN_HASHTABLE_H
#define SN_HASHTABLE_H 1

#include <stdint.h>
#include <inttypes.h>

#include "sn_network.h"

struct sn_hashtable_s;
typedef struct sn_hashtable_s sn_hashtable_t;

sn_hashtable_t *sn_hashtable_create (void);
void sn_hashtable_destroy (sn_hashtable_t *ht);

int sn_hashtable_account (sn_hashtable_t *ht, const sn_network_t *n);
_Bool sn_hashtable_check_collision (sn_hashtable_t *ht, const sn_network_t *n);

uint64_t sn_hashtable_get_collisions (sn_hashtable_t *ht);
double   sn_hashtable_get_collisions_pct (sn_hashtable_t *ht);
uint64_t sn_hashtable_get_networks (sn_hashtable_t *ht);
double   sn_hashtable_get_networks_pct (sn_hashtable_t *ht);
uint64_t sn_hashtable_get_total (sn_hashtable_t *ht);

#endif /* SN_HASHTABLE_H */
/* vim: set sw=2 sts=2 et fdm=marker : */
