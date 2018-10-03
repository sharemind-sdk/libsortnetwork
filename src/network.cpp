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

#include "network.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <list>
#include <stdexcept>
#include <utility>
#include "comparator.h"


namespace sharemind {
namespace SortingNetwork {
namespace {

/* `Glues' two networks together, resulting in a comparator network with twice
 * as many inputs but one that doesn't really sort anymore. It produces a
 * bitonic sequence, though, that can be used by the mergers below. */
Network concatenate(Network const & n0, Network const & n1) {
    auto const inputsInN0 = n0.numInputs();
    assert(std::numeric_limits<decltype(n0.numInputs())>::max() - inputsInN0
           >= n1.numInputs());
    Network n(n0.numInputs() + n1.numInputs());
    auto const stagesInN0 = n0.numStages();
    auto const stagesInN1 = n1.numStages();
    auto const numStages = std::max(stagesInN0, stagesInN1);
    for (std::size_t i = 0u; i < numStages; ++i) {
        auto & stage = n.appendStage();
        if (i < stagesInN0)
            stage.comparators() = n0.stages()[i].comparators();
        if (i < stagesInN1) {
            for (auto const & comp : n1.stages()[i].comparators())
                stage.addComparator(Comparator(comp.min() + inputsInN0,
                                               comp.max() + inputsInN0));
        }
        n.appendStage(std::move(stage));
    }
    return n;
}

struct BitonicMergeJob {
    void execute(Network & n, std::list<BitonicMergeJob> & jobs);
    std::size_t const m_numIndexes;
    std::size_t const m_offset;
    std::size_t const m_skip;
    bool const m_isRecursive;
};

std::list<BitonicMergeJob> bitonicMergerRecursive(std::size_t const numIndexes,
                                                  std::size_t const offset,
                                                  std::size_t const skip)
{
    std::list<BitonicMergeJob> jobs;
    assert(numIndexes > 0u);
    if (numIndexes <= 1u)
        return jobs;

    if (numIndexes > 2u) {
        auto const odd_indizes_num = numIndexes / 2u;
        auto const even_indizes_num = numIndexes - odd_indizes_num;

        jobs.emplace_back(
                    BitonicMergeJob{even_indizes_num, offset, 2u * skip, true});
        jobs.emplace_back(
                    BitonicMergeJob{odd_indizes_num, offset + skip, 2u * skip, true});
    }
    jobs.emplace_back(BitonicMergeJob{numIndexes, offset, skip, false});
    return jobs;
}

void BitonicMergeJob::execute(Network & n, std::list<BitonicMergeJob> & jobs) {
    if (m_isRecursive) {
        jobs.splice(jobs.begin(),
                    bitonicMergerRecursive(m_numIndexes, m_offset, m_skip));
    } else {
        assert(m_numIndexes > 1u);
        for (std::size_t i = 1u; i < m_numIndexes; i += 2u) {
            auto const secondIndex = m_offset + (m_skip * i);
            n.addComparator(Comparator(secondIndex - m_skip, secondIndex));
        }
    }
}

void addBitonicMerger(Network & n, std::size_t const numIndexes) {
    auto jobs(bitonicMergerRecursive(numIndexes, 0u, 1u));
    while (!jobs.empty()) {
        BitonicMergeJob job(std::move(jobs.front()));
        jobs.pop_front();
        job.execute(n, jobs);
    }
}

Network combineBitonicMerge_(Network const & n0, Network const & n1) {
    assert(std::numeric_limits<decltype(n0.numInputs())>::max() - n0.numInputs()
           >= n1.numInputs());

    /* We need to invert n0, because the sequence must be
         z_1 >= z_2 >= ... >= z_k <= z_{k+1} <= ... <= z_p
       and NOT the other way around! Otherwise the comparators added in
       addBitonicMerger() from comparing (z_0,z_1), (z_2,z_3), ... to comparing
       ...,  (z_{n-4},z_{n-3}), (z_{n-2},z_{n-1}), i.e. bound to the end of the
       list, possibly leaving z_0 uncompared. */
    auto n(concatenate(n0.inverted(), n1));
    addBitonicMerger(n, n0.numInputs() + n1.numInputs());
    return n;
}

void addOddEvenMerger(Network & n,
                      std::vector<std::size_t> const & indexesLeft,
                      std::vector<std::size_t> const & indexesRight)
{
    assert(std::numeric_limits<std::size_t>::max() - indexesLeft.size()
           >= indexesRight.size());
    if (indexesLeft.empty() || indexesRight.empty())
        return;

    if ((indexesLeft.size() == 1u) && (indexesRight.size() == 1u)) {
        Comparator c(indexesLeft.front(), indexesRight.front());
        auto & stage = n.appendStage();
        stage.addComparator(std::move(c));
        return;
    }

    {
        /* Merge odd sequences */
        std::vector<std::size_t> tmpLeft(
                    indexesLeft.size() - indexesLeft.size() / 2u);
        for (std::size_t i = 0u; i < tmpLeft.size(); ++i)
            tmpLeft[i] = indexesLeft[2u * i];
        std::vector<std::size_t> tmpRight(
                    indexesRight.size() - indexesRight.size() / 2u);
        for (std::size_t i = 0u; i < tmpRight.size(); ++i)
            tmpRight[i] = indexesRight[2u * i];
        addOddEvenMerger(n, tmpLeft, tmpRight);

        /* Merge even sequences */
        tmpLeft.resize(indexesLeft.size() / 2u);
        for (std::size_t i = 0u; i < tmpLeft.size(); ++i)
            tmpLeft[i] = indexesLeft[2u * i + 1u];
        tmpRight.resize(indexesRight.size() / 2u);
        for (std::size_t i = 0u; i < tmpRight.size(); ++i)
            tmpRight[i] = indexesRight[2u * i + 1u];
        addOddEvenMerger(n, tmpLeft, tmpRight);
    }

    /* Apply ``comparison-interchange'' operations. */
    std::size_t maxIndex = indexesLeft.size() + indexesRight.size();
    assert(maxIndex > 2u);
    if (maxIndex % 2u) {
        maxIndex -= 2u;
    } else {
        maxIndex -= 3u;
    }

    Stage s;
    for (std::size_t i = 1u; i <= maxIndex; i += 2u)
        s.addComparator(
                    Comparator(
                        (i < indexesLeft.size())
                        ? indexesLeft[i]
                        : indexesRight[i - indexesLeft.size()],
                        ((i + 1u) < indexesLeft.size())
                        ? indexesLeft[i + 1u]
                        : indexesRight[i - indexesLeft.size() + 1u]));
    if (!s.empty())
        n.appendStage(std::move(s));
}

Network combineOddEvenMerge_(Network const & n0, Network const & n1) {
    assert(std::numeric_limits<decltype(n0.numInputs())>::max() - n0.numInputs()
           >= n1.numInputs());
    std::vector<std::size_t> indexesLeft(n0.numInputs());
    std::vector<std::size_t> indexesRight(n1.numInputs());
    for (std::size_t i = 0u; i < n0.numInputs(); ++i)
        indexesLeft[i] = i;
    for (std::size_t i = 0u; i < n1.numInputs(); ++i)
        indexesRight[i] = n0.numInputs() + i;

    auto n(concatenate(n0, n1));
    addOddEvenMerger(n, indexesLeft, indexesRight);
    n.compress();
    return n;
}

void createPairwiseInternal(Network & n,
                            std::vector<std::size_t> const & inputs)
{
    for (std::size_t i = 1u; i < inputs.size(); i += 2u)
        n.addComparator(Comparator(inputs[i - 1u], inputs[i]));
    if (inputs.size() <= 2)
        return;

    {
        std::vector<std::size_t> inputsCopy;
        inputsCopy.reserve((inputs.size() + 1u) / 2u);

        /* Sort "pairs" recursively. Like with odd-even mergesort, odd and even
           lines are handled recursively and later reunited. */
        for (std::size_t i = 0u; i < inputs.size(); i += 2u)
            inputsCopy.emplace_back(inputs[i]);
        /* Recursive call #1 with first set of lines */
        createPairwiseInternal(n, inputsCopy);

        inputsCopy.clear();
        for (std::size_t i = 1u; i < inputs.size(); i += 2u)
            inputsCopy.emplace_back(inputs[i]);
        /* Recursive call #2 with second set of lines */
        createPairwiseInternal(n, inputsCopy);
    }

    /* m is the "amplitude" of the sorted pairs. This is a bit tricky to read
       due to different indices being used in the paper, unfortunately. */
    auto m = (inputs.size() + 1u) / 2u;
    while (m > 1u) {
        auto len = m;
        if ((m % 2u) == 0)
            --len;

        for (std::size_t i = 1u; (i + len) < inputs.size(); i += 2u) {
            auto const left = i;
            auto const right = i + len;
            assert(left < right);
            n.addComparator(Comparator(inputs[left], inputs[right]));
        }

        m = (m + 1u) / 2u;
    } /* while (m > 1u) */
}

template <typename Conquer>
Network makeSortWithDivideAndConquer(std::size_t numItems, Conquer & conquer) {
    if (numItems <= 1u)
        return Network(numItems);

    if (numItems == 2u) {
        Network n(2u);
        n.addComparator(Comparator(0u, 1u));
        return n;
    }

    auto const numItemsLeft = numItems / 2u;
    auto const numItemsRight = numItems - numItemsLeft;

    if (numItemsLeft == numItemsRight) {
        auto const nLeft(makeSortWithDivideAndConquer(numItemsLeft, conquer));
        auto r(conquer(nLeft, nLeft));
        r.compress();
        return r;
    } else {
        auto r(conquer(makeSortWithDivideAndConquer(numItemsLeft, conquer),
                       makeSortWithDivideAndConquer(numItemsRight, conquer)));
        r.compress();
        return r;
    }
}

} // anonymous namespace

Network::Network(std::size_t numInputs)
        noexcept(std::is_nothrow_default_constructible<Stages>::value)
    : m_numInputs(numInputs)
{}

Network::Network(Network &&) noexcept = default;
Network::Network(Network const &) = default;

Network Network::makeOddEvenMergeSort(std::size_t numItems)
{ return makeSortWithDivideAndConquer(numItems, combineOddEvenMerge_); }

Network Network::makeBitonicMergeSort(std::size_t numItems)
{ return makeSortWithDivideAndConquer(numItems, combineBitonicMerge_); }

Network Network::makePairwiseSort(std::size_t numItems) {
    Network r(numItems);
    std::vector<std::size_t> inputs(numItems);
    for (std::size_t i = 0; i < numItems; ++i)
        inputs[i] = i;

    createPairwiseInternal(r, inputs);
    r.compress();
    return r;
}

Network::~Network() noexcept = default;

Network & Network::operator=(Network &&) noexcept = default;
Network & Network::operator=(Network const &) = default;

std::size_t Network::numComparators() const noexcept {
    std::size_t r = 0u;
    for (auto const & stage : m_stages)
        r += stage.numComparators();
    return r;
}

void Network::addNetwork(Network other) {
    auto const oldSize = m_stages.size();
    try {
        for (auto & newStage : other.m_stages)
            m_stages.emplace_back(std::move(newStage));
    } catch (...) {
        m_stages.resize(oldSize);
        throw;
    }
}

Stage & Network::appendStage() {
    #if __cplusplus > 201402L
    return m_stages.emplace_back();
    #else
    m_stages.emplace_back();
    return m_stages.back();
    #endif
}

Stage & Network::appendStage(Stage stage) {
    #if __cplusplus > 201402L
    return m_stages.emplace_back(std::move(stage));
    #else
    m_stages.emplace_back(std::move(stage));
    return m_stages.back();
    #endif
}

void Network::removeStage(std::size_t index) {
    assert(index < m_stages.size());
    using D = std::iterator_traits<Stages::iterator>::difference_type;
    m_stages.erase(std::next(m_stages.begin(), static_cast<D>(index)));
}

void Network::addComparator(Comparator comparator) {
    if (!m_stages.empty()) {
        Stage & lastStage = m_stages.back();
        if (lastStage.getConflictsWith(comparator) == Stage::NoConflict)
            return lastStage.addComparator(std::move(comparator));
    }

    appendStage().addComparator(std::move(comparator));
}

void Network::invert() noexcept {
    for (auto & stage : m_stages)
        stage.invert();
}

Network Network::inverted() const noexcept {
    Network r(*this);
    r.invert();
    return r;
}

void Network::shift(std::size_t offset) noexcept {
    if (!offset)
        return;
    for (auto & stage : m_stages)
        stage.shift(offset, m_numInputs);
}

void Network::compress() {
    if (m_stages.empty())
        return;
    for (auto stageIt = std::next(m_stages.begin(), 1);
         stageIt != m_stages.end();
         ++stageIt)
    {
        auto & comps = stageIt->comparators();
        for (std::size_t i = 0u; i < comps.size(); ++i) {
            auto targetStageIt(stageIt);
            for (auto prevStageIt = std::prev(stageIt, 1);; --prevStageIt) {
                auto const conflict =
                        prevStageIt->getConflictsWith(comps[i]);
                if (conflict == Stage::NoConflict) {
                    targetStageIt = prevStageIt;
                } else {
                    if (conflict == Stage::ComparatorAlreadyPresent)
                        targetStageIt = m_stages.end();
                    break;
                }

                if (prevStageIt == m_stages.begin())
                    break;
            }
            if (targetStageIt != stageIt) {
                if (targetStageIt != m_stages.end()) {
                    assert(targetStageIt->getConflictsWith(comps[i])
                           == Stage::NoConflict);
                    targetStageIt->addComparator(std::move(comps[i]));
                }
                stageIt->removeComparator(i);
                --i;
            }
        }
    }

    // Remove all empty stages:
    for (std::size_t i = 0u; i < m_stages.size(); ++i) {
        if (m_stages[i].empty()) {
            m_stages.erase(std::next(m_stages.begin(),
                                     static_cast<Stages::difference_type>(i)));
            --i;
        }
    }
}

Network Network::compressed() const {
    Network n(*this);
    n.compress();
    return n;
}

void Network::normalize() noexcept {
    for (std::size_t i = 0u; i < m_stages.size(); ++i) {
        for (auto const & comparator : m_stages[i].comparators()) {
            auto const min(comparator.min());
            auto const max(comparator.max());
            if (min > max) {
                for (auto j = i; j < m_stages.size(); ++j)
                    m_stages[j].swapIndexes(min, max);
                --i;
                break;
            }
        }
    }
}

Network Network::normalized() const {
    Network n(*this);
    n.normalize();
    return n;
}

void Network::unify() {
    normalize();
    compress();
    for (auto & stage : m_stages)
        stage.unify();
}

Network Network::unified() const {
    Network n(*this);
    n.unify();
    return n;
}

void Network::removeInput(std::size_t index) {
    assert(index < m_numInputs);
    for (auto & stage : m_stages)
        stage.removeInput(index);
    --m_numInputs;
}

Network combineBitonicMerge(Network const & n0, Network const & n1) {
    if (std::numeric_limits<decltype(n0.numInputs())>::max() - n0.numInputs()
        < n1.numInputs())
        throw std::length_error("Resulting comparator network exceeds "
                                "implementation limits!");
    return combineBitonicMerge_(n0, n1);
}

Network combineOddEvenMerge(Network const & n0, Network const & n1) {
    if (std::numeric_limits<decltype(n0.numInputs())>::max() - n0.numInputs()
        < n1.numInputs())
        throw std::length_error("Resulting comparator network exceeds "
                                "implementation limits!");
    return combineOddEvenMerge_(n0, n1);
}

bool Network::bruteForceIsSortingNetwork() const {
    std::vector<std::size_t> sorted(m_numInputs);
    for (std::size_t i = 0u; i < m_numInputs; ++i)
        sorted[i] = i;
    auto maybeUnsorted(sorted);
    do {
        auto test(maybeUnsorted);
        sortValues(test.data());
        if (test != sorted)
            return false;
    } while (std::next_permutation(maybeUnsorted.begin(), maybeUnsorted.end()));
    return true;
}

int Network::compare(Network const & other) const noexcept {
    if (m_numInputs != other.m_numInputs)
        return (m_numInputs < other.m_numInputs) ? -1 : 1;
    if (m_stages.size() != other.m_stages.size())
        return (m_stages.size() < other.m_stages.size()) ? -1 : 1;
    for (std::size_t i = 0u; i < m_stages.size(); ++i)
        if (auto const r = m_stages[i].compare(other.m_stages[i]))
            return r;
    return 0;
}

bool operator<(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) < 0; }

bool operator<=(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) <= 0; }

bool operator==(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) == 0; }

bool operator!=(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) != 0; }

bool operator>=(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) >= 0; }

bool operator>(Network const & lhs, Network const & rhs) noexcept
{ return lhs.compare(rhs) > 0; }

} /* namespace SortingNetwork { */
} /* namespace sharemind { */
