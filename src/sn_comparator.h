/**
 * \file sn_comparator.h
 * \brief The sn_comparator_t class and associated methods.
 *
 * \verbatim
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
 * \endverbatim
 **/

#ifndef SN_COMPARATOR_H
#define SN_COMPARATOR_H 1

/**
 * Struct representing a comparator. Don't access the members of this struct
 * directly, use the macros below instead.
 */
struct sn_comparator_s
{
  int min; /**< Index of the line onto which the smaller element will be put. */
  int max; /**< Index of the line onto which the larger element will be put. */
};
typedef struct sn_comparator_s sn_comparator_t;

/** Returns the "left" line, i.e. the line with the smaller index. */
#define SN_COMP_LEFT(c)  (((c)->min < (c)->max) ? (c)->min : (c)->max)
/** Returns the "right" line, i.e. the line with the larger index. */
#define SN_COMP_RIGHT(c) (((c)->min > (c)->max) ? (c)->min : (c)->max)
/** Returns the index of the line onto which the smaller element will be put. */
#define SN_COMP_MIN(c) (c)->min
/** Returns the index of the line onto which the larger element will be put. */
#define SN_COMP_MAX(c) (c)->max

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
 *   sn_comparator_create().
 */
void sn_comparator_destroy (sn_comparator_t *c);

/**
 * Inverts a comparator by switching the minimum and maximum indexes stored in
 * the comparator.
 *
 * \param c Pointer to the comparator.
 */
void sn_comparator_invert (sn_comparator_t *c);

/**
 * Shifts the indexes stored in the comparator by a constant offset. If the
 * index becomes too large, it will "wrap around".
 *
 * \param c The comparator to modify.
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
 * \param c The comparator to modify.
 * \param con0 Index of the first line.
 * \param con1 Index of the second line.
 */
void sn_comparator_swap (sn_comparator_t *c, int con0, int con1);

/**
 * Compares two comparators and returns less than zero, zero, or greater than
 * zero if the first comparator is smaller than, equal to or larger than the
 * second comparator, respectively.
 *
 * \param c0 Pointer to the first comparator.
 * \param c1 Pointer to the second comparator.
 */
int sn_comparator_compare (const sn_comparator_t *c0,
    const sn_comparator_t *c1);

#endif /* SN_COMPARATOR_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
