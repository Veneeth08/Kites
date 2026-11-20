#include "processor/posit.h"

#include <gtest/gtest.h>

namespace
{
using Kites::posit::Quire256;
using Kites::posit::isQuireNaR;
using Kites::posit::quireMulAdd;
using Kites::posit::quireMulSub;
using Kites::posit::quireNaR;
using Kites::posit::quireToPosit;
using Kites::posit::quireZero;

uint16_t raw(const Quire256 &q)
{
    return static_cast<uint16_t>(quireToPosit(q));
}
}

TEST(Posit16QuireTest, RepresentationAndExactProducts)
{
    EXPECT_EQ(quireZero().limbs, (std::array<uint32_t, 8>{}));
    EXPECT_TRUE(isQuireNaR(quireNaR()));
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0x4000, 0x4000)), 0x4000); // 1
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0xc000, 0x4000)), 0xc000); // -1
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0xc000, 0xc000)), 0x4000); // +1
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0, 0x7fff)), 0);
    EXPECT_EQ(quireMulAdd(quireZero(), 0x0001, 0x0001).limbs[0], 1u);
}

TEST(Posit16QuireTest, AccumulatesSubtractsAndCancelsExactly)
{
    auto q = quireMulAdd(quireZero(), 0x4800, 0x4800); // 2*2
    q = quireMulAdd(q, 0xc000, 0x5000);                // -1*4
    EXPECT_EQ(raw(q), 0);

    q = quireMulAdd(quireZero(), 0x4800, 0x4800);
    EXPECT_EQ(raw(quireMulSub(q, 0x4000, 0x4000)), 0x4c00); // 4 - 1 = 3
}

TEST(Posit16QuireTest, PropagatesNarAndDetectsBothSignedOverflows)
{
    EXPECT_TRUE(isQuireNaR(quireMulAdd(quireNaR(), 0x4000, 0x4000)));
    EXPECT_TRUE(isQuireNaR(quireMulAdd(quireZero(), 0x8000, 0x4000)));
    EXPECT_TRUE(isQuireNaR(quireMulSub(quireZero(), 0x4000, 0x8000)));
    EXPECT_EQ(quireToPosit(quireNaR()), 0x8000);

    Quire256 almostPositiveMax;
    almostPositiveMax.limbs.fill(0xffffffffu);
    almostPositiveMax.limbs.back() = 0x7fffffffu;
    EXPECT_TRUE(isQuireNaR(quireMulAdd(almostPositiveMax, 0x4000, 0x4000)));

    Quire256 almostNegativeMax;
    almostNegativeMax.limbs.fill(0);
    almostNegativeMax.limbs.back() = 0x80000000u;
    almostNegativeMax.limbs[0] = 1; // -2^255 + 1, the most-negative finite value
    EXPECT_TRUE(isQuireNaR(quireMulSub(almostNegativeMax, 0x0001, 0x0001)));
    EXPECT_TRUE(isQuireNaR(quireMulSub(almostNegativeMax, 0x0002, 0x0001)));
}

TEST(Posit16QuireTest, RoundsFiniteQuireValuesThroughPositPath)
{
    EXPECT_EQ(quireToPosit(quireZero()), 0);
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0x4000, 0x4400)), 0x4400); // 1.5
    EXPECT_EQ(raw(quireMulAdd(quireZero(), 0xc000, 0x4400)), 0xbc00); // -1.5

    Quire256 belowMin;
    belowMin.limbs[0] = 1;
    EXPECT_EQ(quireToPosit(belowMin), 0x0001);
    Quire256 aboveMax;
    aboveMax.limbs[7] = 1u << 28; // 2^252 * 2^-112 = 2^140
    EXPECT_EQ(quireToPosit(aboveMax), 0x7fff);

    EXPECT_EQ(quireToPosit(quireMulSub(quireZero(), 0x7fff, 0x7fff)), 0x8001);
}
