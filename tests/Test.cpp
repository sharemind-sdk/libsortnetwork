/*
 * Copyright (C) Cybernetica AS
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
 */

#include "../src/network.h"

#include <cstddef>
#include <sharemind/TestAssert.h>


#if 0
#include <iostream>
template <typename Net>
void printNetwork(Net & net) noexcept {
    std::cout << "Network (size " << net.numInputs() << "):\n";
    for (auto const & stage : net.stages()) {
        std::cout << " Stage:\n";
        for (auto const & comp : stage.comparators())
            std::cout << "  Comp: " << comp.min() << "-" << comp.max() << "\n";
    }
    std::cout << std::flush;
}

#define PRINT_NET(net) printNetwork(net);
#else
#define PRINT_NET(net)
#endif


constexpr std::size_t const sizeLimit = 12u;

template <typename NetworkGenerator>
void testSorting(NetworkGenerator && g) {
    for (std::size_t size = 0u; size < sizeLimit; ++size) {
        auto const net(g(size));
        PRINT_NET(net)
        SHAREMIND_TESTASSERT(net.bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.compressed() == net);
        SHAREMIND_TESTASSERT(net.normalized().bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.normalized().compressed()
                                    .bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.canonicalized().bruteForceIsSortingNetwork());
    }
}

int main() {
    using sharemind::SortingNetwork::Network;
    testSorting(Network::makeBitonicMergeSort);
    testSorting(Network::makeOddEvenMergeSort);
    testSorting(Network::makePairwiseSort);
}
