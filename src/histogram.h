/**
 * libsortnetwork - src/histrogram.h
 * Copyright (C) 2011  Florian octo Forster
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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H 1

struct histogram_s;
typedef struct histogram_s histogram_t;

histogram_t *hist_create (void);
void hist_destroy (histogram_t *h);

int hist_account (histogram_t *h, int rating);
int hist_print (histogram_t *h);

#endif /* HISTOGRAM_H */
