/**
 * libsortnetwork - src/sn_random.h
 * Copyright (C) 2008-2010  Florian octo Forster
 * Copyright (C)      2017  Cybernetica AS
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
 * Modified by Cybernetica AS <sharemind-support at cyber.ee>
 **/

#ifndef SN_RANDOM_H
#define SN_RANDOM_H 1

#ifdef __cplusplus
extern "C" {
#endif

int sn_random_init (void);

int sn_random (void);
int sn_true_random (void);

int sn_bounded_random (int min, int max);
double sn_double_random (void);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* SN_RANDOM_H */
