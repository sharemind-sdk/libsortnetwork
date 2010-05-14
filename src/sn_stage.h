/**
 * libsortnetwork - src/sn_stage.h
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

#ifndef SN_STAGE_H
#define SN_STAGE_H 1

#include <stdio.h>

#include "sn_comparator.h"

struct sn_stage_s
{
  int depth;
  sn_comparator_t *comparators;
  int comparators_num;
};
typedef struct sn_stage_s sn_stage_t;

enum sn_network_cut_dir_e
{
  DIR_MIN,
  DIR_MAX
};

#define SN_STAGE_DEPTH(s) (s)->depth
#define SN_STAGE_COMP_NUM(s) (s)->comparators_num
#define SN_STAGE_COMP_GET(s,n) ((s)->comparators + (n))

sn_stage_t *sn_stage_create (int depth);
sn_stage_t *sn_stage_clone (const sn_stage_t *s);
void sn_stage_destroy (sn_stage_t *s);

int sn_stage_sort (sn_stage_t *s, int *values);

int sn_stage_comparator_add (sn_stage_t *s, const sn_comparator_t *c);
int sn_stage_comparator_remove (sn_stage_t *s, int c_num);
int sn_stage_comparator_check_conflict (sn_stage_t *s, const sn_comparator_t *c);

int sn_stage_show (sn_stage_t *s);
int sn_stage_invert (sn_stage_t *s);
int sn_stage_shift (sn_stage_t *s, int sw, int inputs_num);
int sn_stage_swap (sn_stage_t *s, int con0, int con1);
int sn_stage_cut_at (sn_stage_t *s, int input, enum sn_network_cut_dir_e dir);
int sn_stage_remove_input (sn_stage_t *s, int input);

sn_stage_t *sn_stage_read (FILE *fh);
int sn_stage_write (sn_stage_t *s, FILE *fh);

int sn_stage_serialize (sn_stage_t *s,
    char **ret_buffer, size_t *ret_buffer_size);
sn_stage_t *sn_stage_unserialize (char **buffer, size_t *buffer_size);

#endif /* SN_STAGE_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
