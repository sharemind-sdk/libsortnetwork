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
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
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
 * Creates an empty comparator network and returns a pointer to it. The
 * comparator network must be freed using sn_network_destroy().
 *
 * \param inputs_num Number of inputs the comparator network has.
 * \return Pointer to the comparator network or \c NULL on error.
 */
sn_network_t *sn_network_create (int inputs_num);

/**
 * Clones an existing comparator network.
 *
 * \param n Comparator network to clone.
 * \return Copied sort network or \c NULL on error. The returned network must
 *   be freed using sn_network_destroy().
 */
sn_network_t *sn_network_clone (const sn_network_t *n);

/**
 * Destroys a comparator network allocated with sn_network_create() or one of
 * the other methods returning a \c sn_network_t. This frees all allocated
 * space.
 *
 * \param n The comparator network to destroy. May be \c NULL.
 */
void sn_network_destroy (sn_network_t *n);

/**
 * Creates a new sort network using Batcher's Odd-Even-Mergesort algorithm.
 *
 * \param inputs_num Number of inputs / outputs of the sorting network.
 * \return A pointer to the newly allocated sorting network or \c NULL if an
 *   invalid number of inputs was given or allocation failed.
 */
sn_network_t *sn_network_create_odd_even_mergesort (int inputs_num);

/**
 * Creates a new sort network using Batcher's Bitonic-Mergesort algorithm.
 *
 * \param inputs_num Number of inputs / outputs of the sorting network.
 * \return A pointer to the newly allocated sorting network or \c NULL if an
 *   invalid number of inputs was given or allocation failed.
 */
sn_network_t *sn_network_create_bitonic_mergesort (int inputs_num);

/**
 * Creates a new sorting network using the Pairwise sorting algorithm published
 * by Ian Parberry.
 * \param inputs_num Number of inputs / outputs of the sorting network.
 * \return A pointer to the newly allocated sorting network or \c NULL if an
 *   invalid number of inputs was given or allocation failed.
 */
sn_network_t *sn_network_create_pairwise (int inputs_num);

/**
 * Append another network to a given network.
 *
 * \param n The comparator network to which the other network is added. This
 *   network is modified.
 * \param other The network to be added to the first network. This network is
 *   consumed by this function and the memory pointed to is freed. You cannot
 *   use that network after this call, so use sn_network_clone() if required.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_network_add (sn_network_t *n, sn_network_t *other);

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
 *  on error (\c n is \c NULL).
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
 * \see sn_stage_sort
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

/**
 * Prints the comparator network to \c STDOUT using a human readable
 * representation.
 *
 * \param n The comparator network to display.
 * \return Zero on success, non-zero on failure.
 */
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
 * Removes an input and all comparators touching that input from the comparator
 * network.
 *
 * \param n The network to modify.
 * \param input The zero-based index of the input to remove.
 * \return Zero on success, non-zero on failure.
 */
int sn_network_remove_input (sn_network_t *n, int input);

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

/* FIXME: Documentation */
int sn_network_cut (sn_network_t *n, int *mask);

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
 * \return Newly allocated network with twice the number of inputs or \c NULL
 *   on error.
 */
sn_network_t *sn_network_combine_bitonic_merge (sn_network_t *n0, sn_network_t *n1);

/**
 * Combines two comparator networks using the odd-even-merger.
 *
 * \param n0 First network.
 * \param n1 Second network.
 * \return Newly allocated network or \c NULL on error.
 */
sn_network_t *sn_network_combine_odd_even_merge (sn_network_t *n0, sn_network_t *n1);

/**
 * Reads a comparator network from a open file handle.
 *
 * \param fh The FILE pointer to read from.
 * \return A newly allocated comparator network or \c NULL on error.
 * \see sn_network_read_file
 */
sn_network_t *sn_network_read (FILE *fh);

/**
 * Reads a comparator network from a file.
 *
 * \param file File name to read the network from.
 * \return A newly allocated comparator network or \c NULL on error.
 * \see sn_network_read
 */
sn_network_t *sn_network_read_file (const char *file);

/**
 * Writes a comparator network to a file handle.
 *
 * \param n The comparator network to write.
 * \param fh The file handle to write to. Must be opened in write mode.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_write_file
 */
int sn_network_write (sn_network_t *n, FILE *fh);

/**
 * Writes a comparator network to a file.
 *
 * \param n The comparator network to write.
 * \param fh The file name to write the network to.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_write
 */
int sn_network_write_file (sn_network_t *n, const char *file);

/**
 * Creates a serialized form of the given comparator network. The serialized
 * form is byte-order independent and can easily be sent over a computer
 * network.
 *
 * \param n The comparator network to serialize.
 * \param[out] ret_buffer Pointer to a pointer into which the location of the
 *   allocated memory will be written. It is the caller's responsibility to
 *   free this memory.
 * \param[out] ret_buffer_size Pointer to a size_t into which the size of the
 *   allocated memory will be written.
 * \return Zero on success, non-zero on failure.
 * \see sn_network_unserialize
 */
int sn_network_serialize (sn_network_t *n, char **ret_buffer,
    size_t *ret_buffer_size);

/**
 * Creates a comparator network from its serialized form.
 *
 * \param buffer Pointer to a buffer containing the comparator network in
 *   serialized form.
 * \param buffer_size Size of the buffer (in bytes).
 * \return Pointer to the newly allocated comparator network or \c NULL on
 *   error.
 * \see sn_network_serialize
 */
sn_network_t *sn_network_unserialize (char *buffer, size_t buffer_size);
#endif /* SN_NETWORK_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
