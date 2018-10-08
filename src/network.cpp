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
#include <type_traits>
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
            stage = n0.stage(i);
        if (i < stagesInN1)
            for (auto const & comp : n1.stage(i).comparators())
                stage.addComparator(Comparator(comp.min() + inputsInN0,
                                               comp.max() + inputsInN0));
    }
    return n;
}

struct StrideJob {
    std::size_t const m_numIndexes;
    std::size_t const m_offset;
    std::size_t const m_skip;
    bool const m_isRecursive;
};

std::list<StrideJob> bitonicMergerRecursive(std::size_t const numIndexes,
                                            std::size_t const offset,
                                            std::size_t const skip)
{
    std::list<StrideJob> jobs;
    assert(numIndexes > 0u);
    if (numIndexes <= 1u)
        return jobs;

    if (numIndexes > 2u) {
        auto const odd_indizes_num = numIndexes / 2u;
        auto const even_indizes_num = numIndexes - odd_indizes_num;

        jobs.emplace_back(
                    StrideJob{even_indizes_num, offset, 2u * skip, true});
        jobs.emplace_back(
                    StrideJob{odd_indizes_num, offset + skip, 2u * skip, true});
    }
    jobs.emplace_back(StrideJob{numIndexes, offset, skip, false});
    return jobs;
}

void addBitonicMerger(Network & n, std::size_t const numIndexes) {
    auto jobs(bitonicMergerRecursive(numIndexes, 0u, 1u));
    while (!jobs.empty()) {
        StrideJob const job(std::move(jobs.front()));
        jobs.pop_front();

        if (job.m_isRecursive) {
            jobs.splice(jobs.begin(),
                        bitonicMergerRecursive(job.m_numIndexes,
                                               job.m_offset,
                                               job.m_skip));
        } else {
            assert(job.m_numIndexes > 1u);
            for (std::size_t i = 1u; i < job.m_numIndexes; i += 2u) {
                auto const secondIndex = job.m_offset + (job.m_skip * i);
                n.addComparator(Comparator(secondIndex - job.m_skip,
                                           secondIndex));
            }
        }
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

struct OddEvenMergerJob {
    std::size_t const m_numLeftIndexes;
    std::size_t const m_leftOffset;
    std::size_t const m_leftSkip;
    std::size_t const m_numRightIndexes;
    std::size_t const m_rightOffset;
    std::size_t const m_rightSkip;
    bool const m_isRecursive;
};

std::list<OddEvenMergerJob> oddEvenMergerRecursive(
        std::size_t const numLeftIndexes,
        std::size_t const leftOffset,
        std::size_t const leftSkip,
        std::size_t const numRightIndexes,
        std::size_t const rightOffset,
        std::size_t const rightSkip,
        Network & n)
{
    assert(std::numeric_limits<std::size_t>::max() - numLeftIndexes
           >= numRightIndexes);

    std::list<OddEvenMergerJob> jobs;
    if (!numLeftIndexes || !numRightIndexes)
        return jobs;

    if ((numLeftIndexes == 1u) && (numRightIndexes == 1u)) {
        Comparator c(leftOffset, rightOffset);
        auto & stage = n.appendStage();
        stage.addComparator(std::move(c));
        return jobs;
    }

    {
        /* Merge odd sequences */
        jobs.emplace_back(
                    OddEvenMergerJob{
                        numLeftIndexes - numLeftIndexes / 2u,
                        leftOffset,
                        leftSkip * 2u,
                        numRightIndexes - numRightIndexes / 2u,
                        rightOffset,
                        rightSkip * 2u,
                        true});

        /* Merge even sequences */
        jobs.emplace_back(
                    OddEvenMergerJob{
                        numLeftIndexes / 2u,
                        leftOffset + leftSkip,
                        leftSkip * 2u,
                        numRightIndexes / 2u,
                        rightOffset + rightSkip,
                        rightSkip * 2u,
                        true});
    }

    jobs.emplace_back(
                OddEvenMergerJob{numLeftIndexes,
                                 leftOffset,
                                 leftSkip,
                                 numRightIndexes,
                                 rightOffset,
                                 rightSkip,
                                 false});
    return jobs;
}

void oddEvenMergerNonRecursive(
        std::size_t const numLeftIndexes,
        std::size_t const leftOffset,
        std::size_t const leftSkip,
        std::size_t const numRightIndexes,
        std::size_t const rightOffset,
        std::size_t const rightSkip,
        Network & n)
{
    /* Apply ``comparison-interchange'' operations. */
    std::size_t maxIndex = numLeftIndexes + numRightIndexes;
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
                        (i < numLeftIndexes)
                        ? (leftOffset + i * leftSkip)
                        : (rightOffset + (i - numLeftIndexes) * rightSkip),
                        ((i + 1u) < numLeftIndexes)
                        ? (leftOffset + (i + 1u) * leftSkip)
                        : (rightOffset + (i - numLeftIndexes + 1u) * rightSkip)));
    if (!s.empty())
        n.appendStage(std::move(s));
}

Network combineOddEvenMerge_(Network const & n0, Network const & n1) {
    assert(std::numeric_limits<decltype(n0.numInputs())>::max() - n0.numInputs()
           >= n1.numInputs());
    auto n(concatenate(n0, n1));

    auto jobs(oddEvenMergerRecursive(n0.numInputs(),
                                        0u,
                                        1u,
                                        n1.numInputs(),
                                        n0.numInputs(),
                                        1u,
                                        n));
    while (!jobs.empty()) {
        auto const job(std::move(jobs.front()));
        jobs.pop_front();
        if (job.m_isRecursive) {
            jobs.splice(jobs.begin(),
                        oddEvenMergerRecursive(job.m_numLeftIndexes,
                                                  job.m_leftOffset,
                                                  job.m_leftSkip,
                                                  job.m_numRightIndexes,
                                                  job.m_rightOffset,
                                                  job.m_rightSkip,
                                                  n));
        } else {
            oddEvenMergerNonRecursive(job.m_numLeftIndexes,
                                         job.m_leftOffset,
                                         job.m_leftSkip,
                                         job.m_numRightIndexes,
                                         job.m_rightOffset,
                                         job.m_rightSkip,
                                         n);
        }
    }
    n.compress();
    return n;
}

std::list<StrideJob> pairwiseRecursive(Network & n,
                                       std::size_t const numIndexes,
                                       std::size_t const offset,
                                       std::size_t const skip)
{
    for (std::size_t i = 1u; i < numIndexes; i += 2u) {
        auto const b = offset + (i * skip);
        assert(b < n.numInputs());
        auto const a = b - skip;
        assert(a < n.numInputs());
        n.addComparator(Comparator(a, b));
    }
    std::list<StrideJob> jobs;
    if (numIndexes <= 2)
        return jobs;

    {
        /* Sort "pairs" recursively. Like with odd-even mergesort, odd and even
           lines are handled recursively and later reunited. */

        /* Recursive call #1 with first set of lines */
        auto const numOddIndexes = numIndexes / 2u;
        auto const numEvenIndexes = numIndexes - numOddIndexes;
        jobs.emplace_back(StrideJob{numEvenIndexes, offset, skip * 2u, true});

        /* Recursive call #2 with second set of lines */
        jobs.emplace_back(
                    StrideJob{numOddIndexes, offset + skip, skip * 2u, true});
    }
    jobs.emplace_back(StrideJob{numIndexes, offset, skip, false});
    return jobs;
}

template <typename Conquer>
Network makeSortWithDivideAndConquer(std::size_t numInputs, Conquer & conquer) {
    if (numInputs <= 2u) {
        if (numInputs == 2u) {
            Network n(2u);
            n.addComparator(Comparator(0u, 1u));
            return n;
        }
        return Network(numInputs);
    }

    // Use a stack machine instead of recursion:
    enum DivideAndConquerOp : char { Recurse, ConquerOneDouble, ConquerTwo };
    std::vector<Network> networkStack; // arguments for conquer
    std::vector<DivideAndConquerOp> operationStack;
    operationStack.emplace_back(Recurse);
    std::vector<std::size_t> argumentStack; // arguments for creating networks
    argumentStack.emplace_back(numInputs);

    do {
        auto const operation(operationStack.back());
        operationStack.pop_back();
        if (operation == Recurse) {
            assert(!argumentStack.empty());
            numInputs = argumentStack.back();
            argumentStack.pop_back();
            if (numInputs <= 1u) {
                networkStack.emplace_back(numInputs);
            } else if (numInputs == 2u) {
                networkStack.emplace_back(numInputs);
                networkStack.back().addComparator(Comparator(0u, 1u));
            } else {
                auto const numInputsLeft = numInputs / 2u;
                auto const numInputsRight = numInputs - numInputsLeft;
                if (numInputsLeft == numInputsRight) {
                    argumentStack.emplace_back(numInputsLeft);
                    operationStack.emplace_back(ConquerOneDouble);
                    operationStack.emplace_back(Recurse);
                } else {
                    argumentStack.emplace_back(numInputsRight);
                    argumentStack.emplace_back(numInputsLeft);
                    operationStack.emplace_back(ConquerTwo);
                    operationStack.emplace_back(Recurse);
                    operationStack.emplace_back(Recurse);
                }
            }
        } else if (operation == ConquerOneDouble) {
            assert(!networkStack.empty());
            networkStack.back() = conquer(networkStack.back(),
                                          networkStack.back());
            networkStack.back().compress();
        } else {
            assert(operation == ConquerTwo);
            assert(networkStack.size() >= 2u);
            {
                auto const firstArg(std::move(networkStack.back()));
                networkStack.pop_back();
                networkStack.back() = conquer(firstArg, networkStack.back());
            }
            networkStack.back().compress();
        }
    } while (!operationStack.empty());
    assert(argumentStack.empty());
    assert(networkStack.size() == 1u);
    return std::move(networkStack.front());
}

} // anonymous namespace

Network::Network(std::size_t numInputs) noexcept
    : m_numInputs(numInputs)
{ static_assert(std::is_nothrow_default_constructible<Stages>::value, ""); }

Network::Network(Network &&) noexcept = default;
Network::Network(Network const &) = default;

Network Network::makeOddEvenMergeSort(std::size_t numInputs)
{ return makeSortWithDivideAndConquer(numInputs, combineOddEvenMerge_); }

Network Network::makeBitonicMergeSort(std::size_t numInputs)
{ return makeSortWithDivideAndConquer(numInputs, combineBitonicMerge_); }

Network Network::makePairwiseSort(std::size_t numInputs) {
    Network n(numInputs);
    auto jobs(pairwiseRecursive(n, numInputs, 0u, 1u));
    while (!jobs.empty()) {
        StrideJob job(std::move(jobs.front()));
        jobs.pop_front();
        if (job.m_isRecursive) {
            jobs.splice(jobs.begin(),
                        pairwiseRecursive(n, job.m_numIndexes, job.m_offset, job.m_skip));
        } else {
            /* m is the "amplitude" of the sorted pairs. This is a bit tricky to read
               due to different indices being used in the paper, unfortunately. */
            auto m = (job.m_numIndexes + 1u) / 2u;
            while (m > 1u) {
                auto len = m;
                if ((m % 2u) == 0)
                    --len;

                for (std::size_t i = 1u; i + len < job.m_numIndexes; i += 2u) {
                    auto const left = job.m_offset + (i * job.m_skip);
                    assert(left < n.numInputs());
                    auto const right = job.m_offset + ((i + len) * job.m_skip);
                    assert(left < right);
                    assert(right < n.numInputs());
                    n.addComparator(Comparator(left, right));
                }

                m = (m + 1u) / 2u;
            } /* while (m > 1u) */
        }
    }
    n.compress();
    return n;
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

void Network::swap(Network & other) noexcept {
    static_assert(noexcept(std::swap(m_stages, other.m_stages)), "");
    std::swap(m_numInputs, other.m_numInputs);
    std::swap(m_stages, other.m_stages);
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
