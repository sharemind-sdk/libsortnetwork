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

#include <cstddef>
#include <sharemind/Concepts.h>
#include <sharemind/Iterator.h>
#include <vector>
#include <utility>
#include "comparator.h"


namespace sharemind {
namespace SortingNetwork {

class Stage {

public: /* Types: */

    using Comparators = std::vector<Comparator>;

    enum ConflictType {
        NoConflict = 0,
        Conflict = 1,
        ComparatorAlreadyPresent = 2
    };

public: /* Methods: */

    /** Creates an empty stage. */
    Stage() noexcept;

    Stage(Stage &&) noexcept;
    Stage(Stage const &);

    Stage & operator=(Stage &&) noexcept;
    Stage & operator=(Stage const &);

    ~Stage() noexcept;

    /** \returns whether this stage has no comparators. */
    bool empty() const noexcept { return m_comparators.empty(); }

    /** \returns the number of comparators in this stage. */
    std::size_t numComparators() const noexcept { return m_comparators.size(); }

    /** \returns a reference to the vector of comparators in this stage. */
    Comparators & comparators() noexcept
    { return m_comparators; }

    /** \returns a const reference to the vector of comparators in this stage.*/
    Comparators const & comparators() const noexcept
    { return m_comparators; }

    /**
      Adds a comparator to this stage. If this would return in a conflict (a
      comparator using on of the line already exists in this stage) an error is
      returned.
      \param[in] comparator A reference to the comparator to add.
    */
    void addComparator(Comparator comparator);

    /**
      Removes a comparator from this stage.
      \param[in] index The index of the comparator to remove.
    */
    void removeComparator(std::size_t index) noexcept;

    /**
      Applies this Stage to a range of values.

      \pre The number of values pointed to must be at least the number of
           inputs of the comparator network.
      \param[in] first Iterator to the first value to sort.
    */
    template <typename It,
              SHAREMIND_REQUIRES_CONCEPTS(RandomAccessIterator(It))>
    void sortValues(It first) const {
        for (auto const & c : m_comparators) {
            auto const cMin(c.min());
            auto const cMax(c.max());
            if (first[cMin] > first[cMax])
                std::swap(first[cMin], first[cMax]);
        }
    }

    /**
      Checks whether the given comparator can be added to this stage, i.e. if
      neither line is used by another comparator.
      \param[in] comparator A reference to the comparator.
      \returns The conflict type.
    */
    ConflictType getConflictsWith(Comparator const & comparator);

    /** Inverts this stage by switching the direction of all its comparators. */
    void invert() noexcept;

    /**
      Shifts this stage (permutes the inputs). Each input is shifted \c offset
      positions, higher inputs are "wrapped around".
      \param[in] offset The number of positions to shift.
      \param[in] numInputs The number of inputs of the comparator network. This
                           value is used to "wrap around" inputs. Must be 2 or
                           greater.
    */
    void shift(std::size_t offset, std::size_t numInputs) noexcept;

    void unify();

    /**
      Swaps two lines. This is used by the algorithm in Network::normalize() to
      transform non-standard sort networks to standard sort networks.
      \param[in] index1 Index of the first line.
      \param[in] index2 Index of the second line.
    */
    void swapIndexes(std::size_t index1, std::size_t index2) noexcept;

    /**
      Remove an input from this stage. Removes all touching comparators and
      adapts the input indexes of all remaining comparators.
      \param[in] index The index of the line which to remove.
    */
    void removeInput(std::size_t index) noexcept;

    int compare(Stage const & other) const noexcept;

    void swap(Stage & other) noexcept;

private: /* Fields: */

    /** Comparators contained in this stage: */
    Comparators m_comparators;

};

bool operator<(Stage const & lhs, Stage const & rhs) noexcept;
bool operator<=(Stage const & lhs, Stage const & rhs) noexcept;
bool operator==(Stage const & lhs, Stage const & rhs) noexcept;
bool operator!=(Stage const & lhs, Stage const & rhs) noexcept;
bool operator>=(Stage const & lhs, Stage const & rhs) noexcept;
bool operator>(Stage const & lhs, Stage const & rhs) noexcept;


} /* namespace SortingNetwork { */
} /* namespace sharemind { */

#endif /* SHAREMIND_LIBSORTNETWORK_STAGE_H */
