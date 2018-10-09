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

#include "Comparator.h"

#include <cassert>
#include <utility>


namespace sharemind {
namespace SortingNetwork {

void Comparator::shift(std::size_t offset, std::size_t numInputs) noexcept {
    assert(numInputs >= 2);
    m_min = (m_min + offset) % numInputs;
    m_max = (m_max + offset) % numInputs;
}

void Comparator::swapIndexes(std::size_t index1, std::size_t index2) noexcept {
    if (m_min == index1) {
        m_min = index2;
    } else if (m_min == index2) {
        m_min = index1;
    }

    if (m_max == index1) {
        m_max = index2;
    } else if (m_max == index2) {
        m_max = index1;
    }
}

int Comparator::compare(Comparator const & other) const noexcept {
    {
        auto const l(left());
        auto const otherL(other.left());
        if (l != otherL)
            return (l < otherL) ? -1 : 1;
    }{
        auto const r(right());
        auto const otherR(other.right());
        if (r != otherR)
            return (r < otherR) ? -1 : 1;
    }
    return (0);
}

void Comparator::swap(Comparator & other) noexcept {
    std::swap(m_min, other.m_min);
    std::swap(m_max, other.m_max);
}

bool operator<(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) < 0; }

bool operator<=(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) <= 0; }

bool operator==(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) == 0; }

bool operator!=(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) != 0; }

bool operator>=(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) >= 0; }

bool operator>(Comparator const & lhs, Comparator const & rhs) noexcept
{ return lhs.compare(rhs) > 0; }

} /* namespace SortingNetwork { */
} /* namespace sharemind { */
