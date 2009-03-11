/**
 * collectd - src/sn_network.h
 * Copyright (C) 2008,2009  Florian octo Forster
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

#ifndef SN_NETWORK_H
#define SN_NETWORK_H 1

#include <stdio.h>

#include "sn_comparator.h"
#include "sn_stage.h"

struct sn_network_s
{
  int inputs_num;
  sn_stage_t **stages;
  int stages_num;
};
typedef struct sn_network_s sn_network_t;

#define SN_NETWORK_STAGE_NUM(n) (n)->stages_num
#define SN_NETWORK_STAGE_GET(n,i) ((n)->stages[i])
#define SN_NETWORK_INPUT_NUM(n) (n)->inputs_num

sn_network_t *sn_network_create (int inputs_num);
sn_network_t *sn_network_clone (const sn_network_t *n);
void sn_network_destroy (sn_network_t *n);

sn_network_t *sn_network_create_odd_even_mergesort (int inputs_num);

int sn_network_stage_add (sn_network_t *n, sn_stage_t *s);
int sn_network_stage_remove (sn_network_t *n, int s_num);

int sn_network_comparator_add (sn_network_t *n, const sn_comparator_t *c);

int sn_network_get_comparator_num (const sn_network_t *n);

int sn_network_sort (sn_network_t *n, int *values);
int sn_network_brute_force_check (sn_network_t *n);

int sn_network_show (sn_network_t *n);
int sn_network_invert (sn_network_t *n);
int sn_network_shift (sn_network_t *n, int s);
int sn_network_compress (sn_network_t *n);
int sn_network_normalize (sn_network_t *n);

int sn_network_cut_at (sn_network_t *n, int input, enum sn_network_cut_dir_e dir);
sn_network_t *sn_network_combine (sn_network_t *n0, sn_network_t *n1,
    int is_power_of_two);
sn_network_t *sn_network_combine_bitonic (sn_network_t *n0, sn_network_t *n1);
sn_network_t *sn_network_combine_odd_even_merge (sn_network_t *n0, sn_network_t *n1);

sn_network_t *sn_network_read (FILE *fh);
sn_network_t *sn_network_read_file (const char *file);
int sn_network_write (sn_network_t *n, FILE *fh);
int sn_network_write_file (sn_network_t *n, const char *file);

int sn_network_serialize (sn_network_t *n, char **ret_buffer,
    size_t *ret_buffer_size);
sn_network_t *sn_network_unserialize (char *buffer, size_t buffer_size);
#endif /* SN_NETWORK_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
