/**
 * \file sn_network.h
 * \brief The sn_network_t class and associated methods.
 *
 * This is more details about what this file does.
 *
 * \verbatim
 * libsortnetwork - src/sn_network.h
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


#ifndef SN_NETWORK_H
#define SN_NETWORK_H 1

#include <stdio.h>

#include "sn_comparator.h"
#include "sn_stage.h"

/**
 * The global struct representing a comparator or sort network.
 */
struct sn_network_s
{
  int inputs_num;       /**< Number of inputs of the comparator network. */
  sn_stage_t **stages;  /**< Array of pointers to the stages of a comparator network. */
  int stages_num;       /**< Number of stages in this comparator network. */
};
typedef struct sn_network_s sn_network_t;

#define SN_NETWORK_STAGE_NUM(n) (n)->stages_num
#define SN_NETWORK_STAGE_GET(n,i) ((n)->stages[i])
#define SN_NETWORK_INPUT_NUM(n) (n)->inputs_num

/**
 * Creates an empty comparator network and returns a pointer to it.
 *
 * \param inputs_num Number of inputs the comparator network has.
 * \return Pointer to the comparator network or NULL on error.
 */
sn_network_t *sn_network_create (int inputs_num);

/**
 * Clones an existing comparator network.
 *
 * \param n Comparator network to clone.
 * \return Copied sort network or NULL on error. The returned network must be
 *   freed using sn_network_destroy().
 */
sn_network_t *sn_network_clone (const sn_network_t *n);

/**
 * Destroys a comparator network allocated with sn_network_create() or one of
 * the other methods returning a sn_network_t. This frees all allocated space.
 *
 * \param n The comparator network to destroy. May be NULL.
 */
void sn_network_destroy (sn_network_t *n);

/**
 * Creates a new sort network using Batcher's Odd-Even-Mergesort algorithm.
 *
 * \param inputs_num Number of inputs / outputs of the sorting network.
 * \return A pointer to the newly allocated sorting network or NULL if an
 *   invalid number of inputs was given or allocation failed.
 */
sn_network_t *sn_network_create_odd_even_mergesort (int inputs_num);

/**
 * Append a new stage to a comparator network.
 *
 * \param n The comparator network to which to add the stage.
 * \param s A pointer to a stage. The memory pointed to by this parameter is
 *   not copied and freed by sn_network_destroy(). It is the caller's
 *   responsibility to call sn_stage_clone() if appropriate.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_stage_add (sn_network_t *n, sn_stage_t *s);

/**
 * Remove a stage from a comparator network.
 *
 * \param n A pointer to the comparator network to modify.
 * \param s_num The depth of the comparator network to remove, with zero being
 *   meaning the stage closest to the inputs.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_stage_remove (sn_network_t *n, int s_num);

/**
 * Adds a comparator to a comparator network. The code tries to add the
 * comparator to the last stage, i.e. the stage closest to the outputs. If this
 * is not possible, because one line is used by another comparator, a new stage
 * is created and appended to the sorting network.
 *
 * \param n Pointer to the comparator netork.
 * \param c Pointer to a comparator to add. The given comparator is copied. It
 *   is the caller's responsibility to free c.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_comparator_add (sn_network_t *n, const sn_comparator_t *c);

/**
 * Returns the number of comparators contained in the comparator network. This
 * will traverse all comparators in all stages, resulting in a running time of
 * \f$ \mathcal{O}(n) \f$.
 *
 * \param n Comparator network to work with.
 * \return The number of comparators contained in the network or less than zero
 *  on error (n is NULL).
 */
int sn_network_get_comparator_num (const sn_network_t *n);

/**
 * Applies a comparator network to an array of integers. This is implemented by
 * calling sn_stage_sort() with every stage of the network.
 *
 * \param n Pointer to the comparator netork.
 * \param[in,out] values Pointer to integer values to sort. The number of
 *   integer values pointed to must be at least the number of inputs of the
 *   comparator network. Otherwise, segmentation faults or memory corruption
 *   will occur.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_sort (sn_network_t *n, int *values);

/**
 * Checks whether a given comparator network is a sorting network by testing
 * all \f$ 2^n \f$ 0-1-patterns. Since this function has exponential running
 * time, using it with comparator networks with many inputs is not advisable.
 *
 * \param n The comparator network to test.
 * \return Zero if the comparator network is a sort network, one if the
 *   comparator network is \em not a sort network, or something else on error.
 */
int sn_network_brute_force_check (sn_network_t *n);

int sn_network_show (sn_network_t *n);

/**
 * Inverts a comparator network by switching the direction of all comparators.
 *
 * \param n The network to invert.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_invert (sn_network_t *n);

/**
 * Shifts a comparator network (permutes the inputs). Each input is shifted
 * \f$ s \f$ positions, higher inputs are "wrapped around".
 *
 * \param n The comparator network to shift.
 * \param s The number of positions to shift.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_shift (sn_network_t *n, int s);

/**
 * Compresses a comparator network by moving all comparators to the earliest
 * possible stage and removing all remaining empty stages.
 *
 * \param n The network to compress.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_compress (sn_network_t *n);

/**
 * Converts a non-standard comparator network to a standard comparator network,
 * i.e. a network in which all comparators point in the same direction.
 *
 * \param n The network to normalize.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_normalize (sn_network_t *n);

/**
 * Removes an inputs from a comparator network by assuming positive or negative
 * infinity to be supplied to a given input. The value will take a
 * deterministic way through the comparator network. After removing the path
 * and all comparators it touched from the comparator network, a new comparator
 * network with \f$ n-1 \f$ inputs remains. The remaining outputs behave
 * identical to the original network with respect to one another.
 *
 * \param n The comparator network to modify.
 * \param input The input to remove.
 * \param dir The direction in which to cut, i.e. whether to assume positive or
 *   negative infinity.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_cut_at (sn_network_t *n, int input, enum sn_network_cut_dir_e dir);

/**
 * An alias for sn_network_combine_odd_even_merge().
 */
sn_network_t *sn_network_combine (sn_network_t *n0, sn_network_t *n1);

/**
 * Combines two comparator networks using a bitonic merger. The number of
 * inputs of both comparator networks must be identical and a power of two.
 *
 * \param n0 First network.
 * \param n1 Second network.
 * \return Newly allocated network with twice the number of inputs or NULL on
 *   error.
 */
sn_network_t *sn_network_combine_bitonic_merge (sn_network_t *n0, sn_network_t *n1);

/**
 * Combines two comparator networks using the odd-even-merger.
 *
 * \param n0 First network.
 * \param n1 Second network.
 * \return Newly allocated network or NULL on error.
 */
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
