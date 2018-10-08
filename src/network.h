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

#ifndef SHAREMIND_LIBSORTNETWORK_NETWORK_H
#define SHAREMIND_LIBSORTNETWORK_NETWORK_H

#include <cassert>
#include <cstddef>
#include <sharemind/Concepts.h>
#include <sharemind/Iterator.h>
#include <vector>
#include "comparator.h"
#include "stage.h"


namespace sharemind {
namespace SortingNetwork {

/** Represents a comparator network. */
class Network {

public: /* Types: */

    using Stages = std::vector<Stage>;

public: /* Methods: */

    /**
       Creates an empty sorting network for the given number of inputs.
       \param[in] numInputs The number of inputs to sort.
    */
    Network(std::size_t numInputs) noexcept;

    Network(Network &&) noexcept;
    Network(Network const &);

    /**
      Creates a new sorting network using Batcher's Odd-Even-Mergesort
      algorithm.
      \param[in] numInputs The number of inputs to sort.
    */
    static Network makeOddEvenMergeSort(std::size_t numInputs);

    /**
      Creates a new sort network using Batcher's Bitonic-Mergesort algorithm.
      \param[in] numInputs The number of inputs to sort.
    */
    static Network makeBitonicMergeSort(std::size_t numInputs);

    /**
      Creates a new sorting network using the Pairwise sorting algorithm
      published by Ian Parberry.
      \param[in] numInputs The number of inputs to sort.
    */
    static Network makePairwiseSort(std::size_t numInputs);

    ~Network() noexcept;

    Network & operator=(Network &&) noexcept;
    Network & operator=(Network const &);

    std::size_t numInputs() const noexcept { return m_numInputs; }

    /**
      Adds the given number of inputs to this network.
      \param[in] numInputsToAdd The number of inputs to add.
      \throws std::length_error if the resulting network exceeds implementation
                                limits.
    */
    void addInputs(std::size_t numInputsToAdd);

    Stages const & stages() const noexcept { return m_stages; }

    Stage & stage(std::size_t stageIndex) noexcept
    { return m_stages[stageIndex]; }

    Stage const & stage(std::size_t stageIndex) const noexcept
    { return m_stages[stageIndex]; }

    std::size_t numStages() const noexcept { return m_stages.size(); }

    /** \returns the total amount of comparators in this network. */
    std::size_t numComparators() const noexcept;

    /** \returns the number of comparators for a given stage in this network. */
    std::size_t numComparators(std::size_t stageIndex) const noexcept {
        assert(stageIndex < m_stages.size());
        return m_stages[stageIndex].numComparators();
    }

    /**
      Composes this network with the given network.
      \param[in] network The network to be composed to this network.
    */
    void composeWith(Network && network);

    /**
      Composes this network with the given network.
      \param[in] network The network to be composed to this network.
    */
    void composeWith(Network const & network);

    /**
      Composes this network with a new empty stage.
      \returns a reference to the newly added stage.
    */
    Stage & composeWithEmptyStage();

    /**
      Composes this network with the given stage.
      \param[in] stage The stage to be composed to this network.
      \returns a reference to the newly added stage.
    */
    Stage & composeWith(Stage && stage);

    /**
      Composes this network with the given stage.
      \param[in] stage The stage to be composed to this network.
      \returns a reference to the newly added stage.
    */
    Stage & composeWith(Stage const & stage);

    /**
      Removes a stage from this network.
      \param[in] index The depth of the stage to remove, with the 0-th stage
                       being closest to the inputs.
    */
    void removeStage(std::size_t index);

    /**
      Composes this network with the given comparator. The code tries to add the
      comparator to the last stage, i.e. the stage closest to the outputs. If
      this is not possible, because one line is used by another comparator, a
      new stage with the given comparator is created and composed to the sorting
      network.
      \param[in] comparator The comparator to compose to this network.
     */
    void composeWith(Comparator comparator);

    /** Inverts this network by switching the direction of all comparators. */
    void invert() noexcept;

    /**
       \returns an inverted copy of this network by switching the direction of
                all comparators.
    */
    Network inverted() const noexcept;

    /**
      Shifts this network (permutes the inputs). Each input is shifted by the
      given offset, higher inputs are "wrapped around".
      \param[in] offset The number of positions to shift.
    */
    void shift(std::size_t offset) noexcept;

    /**
      Applies a comparator network to the given array of integers.
      \param[in] first Iterator to the first value to sort.
      \pre The number of values pointed to must be at least the number of inputs
           of the comparator network.
     */
    template <typename It,
              SHAREMIND_REQUIRES_CONCEPTS(RandomAccessIterator(It))>
    void sortValues(It first) const {
        for (auto const & stage : m_stages)
            stage.sortValues(first);
    }

    /**
      Compresses this network by moving all comparators to the earliest possible
      stage and removing all remaining empty stages.
    */
    void compress();

    /**
      Returns a copy of this network on which compress() has been called.
      \returns A compressed version of this network.
    */
    Network compressed() const;

    /**
      Converts a non-standard network to a standard network, i.e. a network in
      which all comparators point in the same direction.
    */
    void normalize() noexcept;

    /**
      Returns a copy of this network on which normalize() has been called.
      \returns A normalized version of this network.
    */
    Network normalized() const;

    void unify();
    Network unified() const;

    /**
      Removes an input and all comparators touching that input from the
      comparator network.
      \param[in] index The index of the input to remove.
    */
    void removeInput(std::size_t index);

    /**
      Checks whether a this network is a sorting network by testing all
      \f$ 2^n \f$ 0-1-patterns. Since this function has exponential running
      time, using it with comparator networks with many inputs is not advisable.
      \returns whether this network is a sorting network.
      \throws std::bad_alloc an out-of-memory condition was encountered.
     */
    bool bruteForceIsSortingNetwork() const;

    /**
      Compares this network with another and returns zero if they are equal. If
      they are not equal, a number greater than zero or less than zero is
      returned in a consistent matter, so this function can be used to sort
      networks and detect duplicates. It is strongly recommended that you call
      unify() on both Networks before comparing them, because their internal
      structure does matter for this function.
      \param[in] other The other network to compare this network with.
      \returns Zero if the two networks are equal, non-zero otherwise. Return
        values are consistent so this function can be used to sort networks.
     */
    int compare(Network const & other) const noexcept;

    void swap(Network & other) noexcept;

private: /* Fields: */

    /** Number of inputs of the comparator network: */
    std::size_t m_numInputs;

    /** Stages of the comparator network: */
    Stages m_stages;

};

bool operator<(Network const & lhs, Network const & rhs) noexcept;
bool operator<=(Network const & lhs, Network const & rhs) noexcept;
bool operator==(Network const & lhs, Network const & rhs) noexcept;
bool operator!=(Network const & lhs, Network const & rhs) noexcept;
bool operator>=(Network const & lhs, Network const & rhs) noexcept;
bool operator>(Network const & lhs, Network const & rhs) noexcept;

/**
  Combines two comparator networks using a bitonic merger.
  \pre The number of inputs of both comparator networks must be identical and a
       power of two.
  \param[in] n0 First network.
  \param[in] n1 Second network.
  \returns The resulting network.
  \throws std::length_error if the resulting network exceeds implementation
                            limits.
  \throws std::bad_alloc an out-of-memory condition was encountered.
 */
Network combineBitonicMerge(Network const & n0, Network const & n1);

/**
  Combines two comparator networks using the odd-even-merger.
  \param[in] n0 First network.
  \param[in] n1 Second network.
  \returns The resulting network.
  \throws std::length_error if the resulting network exceeds implementation
                            limits.
  \throws std::bad_alloc an out-of-memory condition was encountered.
 */
Network combineOddEvenMerge(Network const & n0, Network const & n1);

} /* namespace SortingNetwork { */
} /* namespace sharemind { */

#endif /* SHAREMIND_LIBSORTNETWORK_NETWORK_H */
