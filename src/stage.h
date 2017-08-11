/**
 * \file sn_stage.h
 * \brief The sn_stage_t class and associated methods.
 *
 * \verbatim
 * libsortnetwork - src/sn_stage.h
 * Copyright (C) 2008-2010  Florian octo Forster
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
 * \endverbatim
 **/

#ifndef SN_STAGE_H
#define SN_STAGE_H 1

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

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
  int depth;                    /**< Depth of this stage, where zero means closest to the input. */
  sn_comparator_t *comparators; /**< Pointer to a list of comparators contained in this stage. */
  int comparators_num;          /**< Number of comparators in this stage. */
};
typedef struct sn_stage_s sn_stage_t;

/**
 * Direction into which to cut.
 *
 * \see sn_network_cut_at, sn_stage_cut_at
 */
enum sn_network_cut_dir_e
{
  DIR_MIN, /**< Assume negative infinity. */
  DIR_MAX  /**< Assume positive infinity. */
};

#define SN_STAGE_DEPTH(s) (s)->depth
#define SN_STAGE_COMP_NUM(s) (s)->comparators_num
#define SN_STAGE_COMP_GET(s,n) ((s)->comparators + (n))

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
 * \return Zero on success, non-zero on failure.
 * \see sn_network_sort
 */
int sn_stage_sort (sn_stage_t *s, int *values);

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

/**
 * Checks whether the given comparator can be added to a stage, i.e. if neither
 * line if used by another comparator.
 *
 * \param s Pointer to the stage.
 * \param c Pointer to the comparator.
 * \return Zero if there is no conflict, one if there is a conflict and two if
 *   the comparator is already present in the stage.
 */
int sn_stage_comparator_check_conflict (sn_stage_t *s, const sn_comparator_t *c);

/**
 * Prints the stage to \c STDOUT using a human readable representation.
 *
 * \param s The comparator network to display.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_show
 */
int sn_stage_show (sn_stage_t *s);
int sn_stage_show_fh (sn_stage_t *s, FILE *fh);

/**
 * Inverts a stage by switching the direction of all comparators.
 *
 * \param s The stage to invert.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_invert
 */
int sn_stage_invert (sn_stage_t *s);

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
 * Removes an input / line by assuming positive or negative infinity is applied
 * to one line.
 *
 * \param s The stage to work with.
 * \param input The input / line on which is assumed to be positive or negative
 *   infinity.
 * \param dir Direction in which to cut; whether positive or negative infinity
 *   is assumed.
 * \return The new position of the infinite value on success or less than zero
 *   on failure.
 * \see sn_network_cut_at
 */
int sn_stage_cut_at (sn_stage_t *s, int input, enum sn_network_cut_dir_e dir);

/* FIXME: Documentation */
int sn_stage_cut (sn_stage_t *s, int *mask, sn_stage_t **prev);

/**
 * Remove an input from a stage, remove all touching comparators and adapt the
 * input indexes of all remaining comparators.
 *
 * \param s The stage from which to remove the input.
 * \param input The index of the line which to remove.
 * \return Zero on success, non-zero on failure.
 */
int sn_stage_remove_input (sn_stage_t *s, int input);

/**
 * Reads a stage from a \c FILE pointer and return the newly allocated stage.
 *
 * \param fh The file handle.
 * \return Pointer to a newly allocated stage or \c NULL on error.
 * \see sn_stage_write
 */
sn_stage_t *sn_stage_read (FILE *fh);

/**
 * Writes a stage to a \c FILE pointer.
 *
 * \param s The stage to write.
 * \param fh The file handle to write to.
 * \return Zero on success, non-zero on failure.
 * \see sn_stage_read
 */
int sn_stage_write (sn_stage_t *s, FILE *fh);

/**
 * Creates a serialized form of the given stage. The serialized form is
 * byte-order independent and can easily be sent over a computer network.
 *
 * \param s The stage to serialize.
 * \param[out] ret_buffer Pointer to a pointer into which the location of the
 *   allocated memory will be written. It is the caller's responsibility to
 *   free this memory.
 * \param[out] ret_buffer_size Pointer to a size_t into which the size of the
 *   allocated memory will be written.
 * \return Zero on success, non-zero on failure.
 * \see sn_stage_unserialize(), sn_network_serialize()
 */
int sn_stage_serialize (sn_stage_t *s,
    char **ret_buffer, size_t *ret_buffer_size);

/**
 * Creates a stage from its serialized form.
 *
 * \param buffer Pointer to a buffer containing the stage in serialized form.
 * \param buffer_size Size of the buffer (in bytes).
 * \return Pointer to the newly allocated stage or \c NULL on error.
 * \see sn_stage_serialize(), sn_network_unserialize()
 */
sn_stage_t *sn_stage_unserialize (char **buffer, size_t *buffer_size);

int sn_stage_compare (const sn_stage_t *s0, const sn_stage_t *s1);

uint64_t sn_stage_get_hashval (const sn_stage_t *s);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* SN_STAGE_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
