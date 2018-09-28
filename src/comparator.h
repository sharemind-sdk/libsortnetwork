/*
 * Copyright (C) 2008-2010  Florian octo Forster
 * Copyright (C) 2017-2018  Cybernetica AS
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
 */

#ifndef SHAREMIND_LIBSORTNETWORK_COMPARATOR_H
#define SHAREMIND_LIBSORTNETWORK_COMPARATOR_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Struct representing a comparator. Don't access the members of this struct
 * directly, use the macros below instead.
 */
struct sn_comparator_s
{
  int m_min; /**< Index of the line onto which the smaller element will be put. */
  int m_max; /**< Index of the line onto which the larger element will be put. */
};
typedef struct sn_comparator_s sn_comparator_t;

/** Returns the "left" line, i.e. the line with the smaller index. */
#define SN_COMP_LEFT(c)  (((c)->m_min < (c)->m_max) ? (c)->m_min : (c)->m_max)
/** Returns the "right" line, i.e. the line with the larger index. */
#define SN_COMP_RIGHT(c) (((c)->m_min > (c)->m_max) ? (c)->m_min : (c)->m_max)
/** Returns the index of the line onto which the smaller element will be put. */
#define SN_COMP_MIN(c) (c)->m_min
/** Returns the index of the line onto which the larger element will be put. */
#define SN_COMP_MAX(c) (c)->m_max

/**
 * Initializes a new comparator object.
 *
 * \param c Pointer to the comparator to initialize, must be non-NULL.
 * \param min Index of the line onto which the smaller element will be put.
 * \param max Index of the line onto which the larger element will be put.
 */
void sn_comparator_init(sn_comparator_t * c, int min, int max);

/**
 * Allocates, initializes and returns a new comparator object. The returned
 * pointer must be freed by sn_comparator_destroy().
 *
 * \param min Index of the line onto which the smaller element will be put.
 * \param max Index of the line onto which the larger element will be put.
 * \return The newly allocated object or \c NULL on failure.
 */
sn_comparator_t *sn_comparator_create (int min, int max);

/**
 * Destroys a comparator object, freeing all allocated space.
 *
 * \param c Pointer to the comparator object. This pointer must be allocated by
 *   sn_comparator_create(), and must be non-NULL.
 */
void sn_comparator_destroy (sn_comparator_t *c);

/**
 * Inverts a comparator by switching the minimum and maximum indexes stored in
 * the comparator.
 *
 * \param c Pointer to the comparator, must be non-NULL.
 */
void sn_comparator_invert (sn_comparator_t *c);

/**
 * Shifts the indexes stored in the comparator by a constant offset. If the
 * index becomes too large, it will "wrap around".
 *
 * \param c The comparator to modify, must be non-NULL.
 * \param sw The offset by which to shift the indexes.
 * \param inputs_num The number of lines / inputs of the comparator network.
 *   This number is used to wrap large indexes around.
 */
void sn_comparator_shift (sn_comparator_t *c, int sw, int inputs_num);

/**
 * Swaps two line indexes by replacing all occurrences of one index with
 * another index and vice versa. If the comparator does not touch either line,
 * this is a no-op.
 *
 * \param c The comparator to modify, must be non-NULL.
 * \param con0 Index of the first line.
 * \param con1 Index of the second line.
 */
void sn_comparator_swap (sn_comparator_t *c, int con0, int con1);

/**
 * Compares two comparators and returns less than zero, zero, or greater than
 * zero if the first comparator is smaller than, equal to or larger than the
 * second comparator, respectively.
 *
 * \param c0 Pointer to the first comparator, must be non-NULL.
 * \param c1 Pointer to the second comparator, must be non-NULL.
 */
int sn_comparator_compare (const sn_comparator_t *c0,
    const sn_comparator_t *c1);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* SHAREMIND_LIBSORTNETWORK_COMPARATOR_H */
