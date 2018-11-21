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

#include <cstddef>
#include <utility>


namespace sharemind {
namespace SortingNetwork {

class Comparator {

public: /* Methods: */

    /**
      \param[in] min Index of the line onto which the smaller element will be
                     put.
      \param[in] max Index of the line onto which the larger element will be
                     put.
    */
    constexpr Comparator(std::size_t min, std::size_t max) noexcept
        : m_min(min)
        , m_max(max)
    {}

    constexpr Comparator(Comparator &&) noexcept = default;
    constexpr Comparator(Comparator const &) noexcept = default;

    constexpr Comparator & operator=(Comparator && move) noexcept = default;
    constexpr Comparator & operator=(Comparator const & copy) noexcept
        = default;

    /** \returns the "left" line, i.e. the line with the smaller index. */
    constexpr std::size_t left() const noexcept
    { return (m_min < m_max) ? m_min : m_max; }

    /** \returns the "right" line, i.e. the line with the larger index. */
    constexpr std::size_t right() const noexcept
    { return (m_min > m_max) ? m_min : m_max; }

    /** \returns the index of the line onto which the smaller element will be
                 put. */
    constexpr std::size_t min() const noexcept { return m_min; }

    /**
       Sets the index of the line onto which the smaller element will be put.
       \param[in] newValue The new index.
    */
    void setMin(std::size_t newValue) noexcept { m_min = newValue; }

    /** \returns the index of the line onto which the larger element will be
                 put. */
    constexpr std::size_t max() const noexcept { return m_max; }

    /**
       Sets the index of the line onto which the larger element will be put.
       \param[in] newValue The new index.
    */
    void setMax(std::size_t newValue) noexcept { m_max = newValue; }

    /**
      Inverts the comparator by switching the minimum and maximum indexes stored
      in the comparator.
    */
    void invert() noexcept { std::swap(m_min, m_max); }

    /**
      Shifts the indexes stored in the comparator by a constant offset. If the
      index becomes too large, it will "wrap around".
      \param[in] offset The offset by which to shift the indexes.
      \param[in] numInputs The number of lines / inputs of the comparator
                           network, a number used to wrap large indexes around.
                           Must be 2 or greater.
    */
    void shift(std::size_t offset, std::size_t numInputs) noexcept;

    /**
      Swaps two line indexes by replacing all occurrences of one index with
      another index and vice versa. If the comparator does not touch either
      line, this is a no-op.
      \param[in] index1 Index of the first line.
      \param[in] index2 Index of the second line.
    */
    void swapIndexes(std::size_t index1, std::size_t index2) noexcept;

    /**
      \returns a value less than, equal to, or greater than zero if this
               comparator is respectively smaller than, equal to or larger than
               the given argument.
      \param[in] other Reference to the other comparator.
    */
    int compare(Comparator const & other) const noexcept;

    void swap(Comparator & other) noexcept;

private: /* Fields: */

    /** Index of the line onto which the smaller element will be put: */
    std::size_t m_min;

    /** Index of the line onto which the larger element will be put: */
    std::size_t m_max;

};

bool operator<(Comparator const & lhs, Comparator const & rhs) noexcept;
bool operator<=(Comparator const & lhs, Comparator const & rhs) noexcept;
bool operator==(Comparator const & lhs, Comparator const & rhs) noexcept;
bool operator!=(Comparator const & lhs, Comparator const & rhs) noexcept;
bool operator>=(Comparator const & lhs, Comparator const & rhs) noexcept;
bool operator>(Comparator const & lhs, Comparator const & rhs) noexcept;

} /* namespace SortingNetwork { */
} /* namespace sharemind { */

#endif /* SHAREMIND_LIBSORTNETWORK_COMPARATOR_H */
