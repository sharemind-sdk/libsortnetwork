/**
 * libsortnetwork - src/sn_comparator.h
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

#ifndef SN_COMPARATOR_H
#define SN_COMPARATOR_H 1

struct sn_comparator_s
{
  int min;
  int max;
};
typedef struct sn_comparator_s sn_comparator_t;

#define SN_COMP_LEFT(c)  (((c)->min < (c)->max) ? (c)->min : (c)->max)
#define SN_COMP_RIGHT(c) (((c)->min > (c)->max) ? (c)->min : (c)->max)
#define SN_COMP_MIN(c) (c)->min
#define SN_COMP_MAX(c) (c)->max

sn_comparator_t *sn_comparator_create (int min, int max);
void sn_comparator_destroy (sn_comparator_t *c);
void sn_comparator_invert (sn_comparator_t *c);
void sn_comparator_shift (sn_comparator_t *c, int sw, int inputs_num);
void sn_comparator_swap (sn_comparator_t *c, int con0, int con1);

int sn_comparator_compare (const void *, const void *);

#endif /* SN_COMPARATOR_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
