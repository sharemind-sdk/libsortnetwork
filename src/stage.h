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

#ifndef SHAREMIND_LIBSORTNETWORK_STAGE_H
#define SHAREMIND_LIBSORTNETWORK_STAGE_H

#include "comparator.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Struct representing a stage of the comparator network. Don't modify this
 * struct yourself, use the sn_stage_* methods instead.
 */
struct sn_stage_s
{
  int m_depth;                    /**< Depth of this stage, where zero means closest to the input. */
  sn_comparator_t *m_comparators; /**< Pointer to a list of comparators contained in this stage. */
  int m_comparators_num;          /**< Number of comparators in this stage. */
};
typedef struct sn_stage_s sn_stage_t;

#define SN_STAGE_DEPTH(s) (s)->m_depth
#define SN_STAGE_COMP_NUM(s) (s)->m_comparators_num
#define SN_STAGE_COMP_GET(s,n) ((s)->m_comparators + (n))

/**
 * Creates an empty stage and returns a pointer to it. The stage must be freed
 * using sn_stage_destroy().
 *
 * \param depth Depth of the stage within the comparator network. This will be
 *   re-set by sn_network_stage_add().
 * \return Pointer to the comparator network or \c NULL on error.
 */
sn_stage_t *sn_stage_create (int depth);

/**
 * Clones an existing stage.
 *
 * \param s Stage to clone.
 * \return Copied stage or \c NULL on error. The returned stage must be freed
 *   using sn_stage_destroy().
 */
sn_stage_t *sn_stage_clone (const sn_stage_t *s);

/**
 * Destroys a stage allocated with sn_stage_create() or one of the other
 * methods returning a \c sn_stage_t. This frees all allocated space.
 *
 * \param s The stage to destroy. May be \c NULL.
 */
void sn_stage_destroy (sn_stage_t *s);

/**
 * Applies a stage to an array of integers.
 *
 * \param s Pointer to the stage.
 * \param[in,out] values Pointer to integer values to sort. The number of
 *   integer values pointed to must be at least the number of inputs of the
 *   comparator network. Otherwise, segmentation faults or memory corruption
 *   will occur.
 * \see sn_network_sort
 */
void sn_stage_sort (sn_stage_t *s, int *values);

/**
 * Adds a comparator to a stage. If this would return in a conflict (a
 * comparator using on of the line already exists in this stage) an error is
 * returned.
 *
 * \param s Pointer to the stage.
 * \param c Pointer to a comparator to add. The given comparator is copied. It
 *   is the caller's responsibility to free c.
 * \return Zero on success, non-zero on failure.
 * \see sn_stage_comparator_remove
 */
int sn_stage_comparator_add (sn_stage_t *s, const sn_comparator_t *c);


/**
 * Removed a comparator from a stage.
 *
 * \param s The stage from which to remove the comparator.
 * \param index The index of the comparator to remove.
 * \return Zero on success, non-zero on failure.
 * \see sn_stage_comparator_add
 */
int sn_stage_comparator_remove (sn_stage_t *s, int index);

typedef enum {
    SN_NO_CONFLICT = 0,
    SN_CONFLICT = 1,
    SN_COMPARATOR_ALREADY_PRESENT = 2
} sn_conflict_type;

/**
 * Checks whether the given comparator can be added to a stage, i.e. if neither
 * line if used by another comparator.
 *
 * \param s Pointer to the stage.
 * \param c Pointer to the comparator.
 * \returns The conflict type.
 */
sn_conflict_type sn_stage_comparator_check_conflict(sn_stage_t * s,
                                                    sn_comparator_t const * c);

/**
 * Inverts a stage by switching the direction of all comparators.
 *
 * \param s The stage to invert, must be non-NULL.
 * \see sn_network_invert
 */
void sn_stage_invert (sn_stage_t *s);

/**
 * Shifts a stage (permutes the inputs). Each input is shifted \c sw positions,
 * higher inputs are "wrapped around".
 *
 * \param s The stage to shift.
 * \param sw The number of positions to shift.
 * \param inputs_num The number of inputs of the comparator network. This value
 *   is used to "wrap around" inputs.
 * \return Zero on success, non-zero on failure.
 */
int sn_stage_shift (sn_stage_t *s, int sw, int inputs_num);

int sn_stage_unify (sn_stage_t *s);

/**
 * Swaps two lines. This is used by the algorithm used in
 * sn_network_normalize() to transform non-standard sort networks to standard
 * sort networks.
 *
 * \param s The stage on which to operate.
 * \param con0 Index of the first line.
 * \param con1 Index of the second line.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_normalize(), sn_comparator_swap()
 */
int sn_stage_swap (sn_stage_t *s, int con0, int con1);

/**
 * Remove an input from a stage, remove all touching comparators and adapt the
 * input indexes of all remaining comparators.
 *
 * \param s The stage from which to remove the input.
 * \param input The index of the line which to remove.
 * \return Zero on success, non-zero on failure.
 */
int sn_stage_remove_input (sn_stage_t *s, int input);

int sn_stage_compare (const sn_stage_t *s0, const sn_stage_t *s1);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* SHAREMIND_LIBSORTNETWORK_STAGE_H */
