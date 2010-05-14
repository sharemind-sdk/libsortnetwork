/**
 * libsortnetwork - src/pop_stats.h
 * Copyright (C) 2009-2010  Florian octo Forster
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

#ifndef POP_STATS_H
#define POP_STATS_H 1

struct pop_stats_s;
typedef struct pop_stats_s pop_stats_t;

pop_stats_t *pop_stats_create (void);
void pop_stats_destroy (pop_stats_t *);

int pop_stats_opt_file (pop_stats_t *, const char *file);
int pop_stats_opt_interval (pop_stats_t *, long interval);

int pop_stats_add_rating (pop_stats_t *, int);

#endif /* POP_STATS_H */

/* vim: set shiftwidth=2 softtabstop=2 et : */
