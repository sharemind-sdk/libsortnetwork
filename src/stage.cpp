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

#include "stage.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>
#include <utility>
#include "comparator.h"


namespace sharemind {
namespace SortingNetwork {

Stage::Stage() noexcept
{ static_assert(std::is_nothrow_default_constructible<Comparators>::value,""); }

Stage::~Stage() noexcept = default;

Stage::Stage(Stage &&) noexcept = default;
Stage::Stage(Stage const &) = default;

Stage & Stage::operator=(Stage &&) noexcept = default;
Stage & Stage::operator=(Stage const &) = default;

void Stage::addComparator(Comparator comparator) {
    assert(getConflictsWith(comparator) == NoConflict);
    auto posIt(std::lower_bound(m_comparators.begin(),
                                m_comparators.end(),
                                comparator));
    m_comparators.insert(std::move(posIt), std::move(comparator));
    assert(std::is_sorted(m_comparators.begin(), m_comparators.end()));
}

void Stage::removeComparator(std::size_t index) noexcept {
    assert(index < m_comparators.size());
    m_comparators.erase(
                std::next(m_comparators.begin(),
                          static_cast<Comparators::difference_type>(index)));
}

Stage::ConflictType Stage::getConflictsWith(Comparator const & c) {
    auto const cMin(c.min());
    auto const cMax(c.max());
    for (auto const & c2: m_comparators) {
        auto const c2Min(c2.min());

        if (cMin == c2Min) {
            auto const c2Max(c2.max());
            if (cMax == c2Max)
                return ComparatorAlreadyPresent;
            return Conflict;
        } else {
            if (cMax == c2Min)
                return Conflict;
            auto const c2Max(c2.max());
            if ((cMin == c2Max) || (cMax == c2Max))
                return Conflict;
        }
    }

  return NoConflict;
}

void Stage::invert() noexcept {
    for (auto & comp : m_comparators)
        comp.invert();
}

void Stage::shift(std::size_t offset, std::size_t numInputs) noexcept {
    if (numInputs < 2)
        return;
    offset %= numInputs;
    if (offset == 0)
        return;
    for (auto & comp : m_comparators)
        comp.shift(offset, numInputs);
}

void Stage::canonicalize()
{ std::sort(m_comparators.begin(), m_comparators.end()); }

void Stage::swapIndexes(std::size_t index1, std::size_t index2) noexcept {
    for (auto & comp : m_comparators)
        comp.swapIndexes(index1, index2);
}

void Stage::removeInput(std::size_t input) noexcept {
    for (std::size_t i = 0u; i < m_comparators.size(); ++i) {
        auto & c = m_comparators[i];
        if ((c.min() == input) || (c.max() == input)) {
            removeComparator(i);
            --i;
        } else {
            if (c.min() > input)
                c.setMin(c.min() - 1u);
            if (c.max() > input)
                c.setMax(c.max() - 1u);
        }

    }
}

int Stage::compare(Stage const & other) const noexcept {
    if (m_comparators.size() != other.m_comparators.size())
        return (m_comparators.size() < other.m_comparators.size()) ? -1 : 1;

    auto otherIt(other.m_comparators.begin());
    for (auto const & comp : m_comparators) {
        auto const r(comp.compare(*otherIt));
        if (r != 0)
            return r;
        ++otherIt;
    }
    return 0;
}

void Stage::swap(Stage & other) noexcept {
    static_assert(noexcept(std::swap(m_comparators, other.m_comparators)), "");
    std::swap(m_comparators, other.m_comparators);
}

bool operator<(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) < 0; }

bool operator<=(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) <= 0; }

bool operator==(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) == 0; }

bool operator!=(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) != 0; }

bool operator>=(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) >= 0; }

bool operator>(Stage const & lhs, Stage const & rhs) noexcept
{ return lhs.compare(rhs) > 0; }

} /* namespace SortingNetwork { */
} /* namespace sharemind { */
