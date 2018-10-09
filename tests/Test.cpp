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

#include "../src/Network.h"

#include <cstddef>
#include <ios>
#include <sharemind/TestAssert.h>
#include <sstream>
#include <string>


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

namespace {

char const * const expectedBitonicMergeSortNetworks[] = {
    "10",
    "11",
    "12S1011",
    "13S1112S1012S1011",
    "14S11101213S10121113S10111213",
    "15S11101314S1214S10141213S10121113S10111213",
    "16S12111415S12101315S11101314S10141115S101211131415S10111213",
    "17S121114131516S121013151416S111013141516S101411151216S101211131416S101112"
        "131415",
    "18S1011131215141617S1210131114161517S1110131214151617S1014111512161317S101"
        "2111314161517S1011121314151617",
    "19S1011131215141718S121013111618S1110131214181617S101814161517S14151617S10"
        "14111512161317S1012111314161517S1011121314151617",
    "1aS1011141316151819S14121719S1410131215191718S1210131115171618S11101312151"
        "61718S1018111912161317S101411151819S1012111314161517S1011121314151617",
    "1bS101114131716191aS14121715181aS1410131216151819S121013111519161aS1110131"
        "215171618191aS1119121a15161718S1018111512161317S101411131517181aS10121"
        "4161819S1011121314151617"
};
char const * const expectedOddEvenMergeSortNetworks[] = {
    "10",
    "11",
    "12S1011",
    "13S1112S1011S1112",
    "14S10111213S10121113S1112",
    "15S10111314S1213S10121314S11131214S11121314",
    "16S11121415S10111314S101311121415S11141215S1213S11121314",
    "17S111213141516S101113151416S101311121415S11141215S12131416S111213141516",
    "18S1011121314151617S1012111314161517S1014111213171516S11151216S12141315S11"
        "1213141516",
    "19S1011121314151718S101211131617S111214161718S101415171618S15161718S111512"
        "1613171418S121413151618S1112131415161718",
    "1aS1011131415161819S12131718S1012131415171819S10151113121416181719S1112131"
        "416171819S1116121713181419S13161415S12141517S1112131415161718",
    "1bS101113141617191aS121315161819S1012131415181617191aS1015111312141619171a"
        "S111213141718S16171819S1116121713181419S1415161aS121413161517181aS1112"
        "131415161718191a"
};
char const * const expectedPairwiseSortNetworks[] = {
    "10",
    "11",
    "12S1011",
    "13S1011S1012S1112",
    "14S10111213S10121113S1112",
    "15S10111213S10121113S1014S1214S1114S11121314",
    "16S101112131415S10121113S10141115S12141315S1114S11121314",
    "17S101112131415S101211131416S101411151216S12141315S11141316S111213141516",
    "18S1011121314151617S1012111314161517S1014111512161317S12141315S11141316S11"
        "1213141516",
    "19S1011121314151617S1012111314161517S1014111512161317S10181315S1418S1218S1"
        "2141618S11161318S111413161518S1112131415161718",
    "1aS10111213141516171819S1012111314161517S1014111512161317S10181119S1418151"
        "9S12181319S1214131516181719S11161318S111413161518S1112131415161718",
    "1bS10111213141516171819S1012111314161517181aS1014111512161317S10181119121a"
        "S14181519161aS12181319S1214131516181719S11161318151aS111413161518171aS"
        "1112131415161718191a"
};

std::string serialize(sharemind::SortingNetwork::Network const & net) noexcept {
    static auto const numHexDigits =
            [](std::size_t number) noexcept -> std::size_t {
                if (number == 0u)
                    return 1u;
                std::size_t digits = 0u;
                for (; number; ++digits)
                    number /= 16u;
                return digits;
            };

    std::ostringstream oss;
    oss << std::hex << numHexDigits(net.numInputs()) << net.numInputs();
    for (auto const & stage : net.stages()) {
        oss << 'S';
        for (auto const & comp : stage.comparators())
            oss << numHexDigits(comp.min()) << comp.min()
                << numHexDigits(comp.max()) << comp.max();
    }
    return oss.str();
}


constexpr std::size_t const sizeLimit = 12u;

template <typename NetworkGenerator>
void testGenerator(NetworkGenerator && g, char const * const * const expected) {
    for (std::size_t size = 0u; size < sizeLimit; ++size) {
        auto const net(g(size));
        PRINT_NET(net)
        SHAREMIND_TESTASSERT(serialize(net) == expected[size]);
        SHAREMIND_TESTASSERT(net.bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.compressed() == net);
        SHAREMIND_TESTASSERT(net.normalized().bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.normalized().compressed()
                                    .bruteForceIsSortingNetwork());
        SHAREMIND_TESTASSERT(net.canonicalized().bruteForceIsSortingNetwork());
    }
}

} // anonymous namespace

int main() {
    using sharemind::SortingNetwork::Network;
    testGenerator(Network::makeBitonicMergeSort,
                  expectedBitonicMergeSortNetworks);
    testGenerator(Network::makeOddEvenMergeSort,
                  expectedOddEvenMergeSortNetworks);
    testGenerator(Network::makePairwiseSort,
                  expectedPairwiseSortNetworks);
}
