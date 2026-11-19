#include "processor/posit.h"

#include <gtest/gtest.h>

TEST(Posit16Test, DecodesSpecialAndBoundaryValues)
{
    const auto zero = Kites::posit::decode(0x0000);
    EXPECT_FALSE(zero.nar);
    EXPECT_EQ(zero.significand, 0);

    EXPECT_TRUE(Kites::posit::decode(0x8000).nar);

    const auto minpos = Kites::posit::decode(0x0001);
    EXPECT_FALSE(minpos.negative);
    EXPECT_EQ(minpos.significand, 1);
    EXPECT_EQ(minpos.scale, -56);

    const auto maxpos = Kites::posit::decode(0x7fff);
    EXPECT_EQ(maxpos.significand, 1);
    EXPECT_EQ(maxpos.scale, 56);

    const auto negativeOne = Kites::posit::decode(0xc000);
    EXPECT_TRUE(negativeOne.negative);
    EXPECT_EQ(negativeOne.significand, 2048);
    EXPECT_EQ(negativeOne.scale, -11);
}

TEST(Posit16Test, EncodesKnownPowersAndSigns)
{
    using Kites::posit::encode;
    EXPECT_EQ(encode(false, 0, 0), 0x0000);
    EXPECT_EQ(encode(false, 1, 0), 0x4000);  // 1
    EXPECT_EQ(encode(false, 1, 1), 0x4800);  // 2
    EXPECT_EQ(encode(false, 1, 2), 0x5000);  // 4
    EXPECT_EQ(encode(false, 1, -1), 0x3800); // 1/2
    EXPECT_EQ(encode(true, 1, 0), 0xc000);   // -1
}

TEST(Posit16Test, EveryFiniteEncodingRoundTrips)
{
    for (uint32_t raw = 0; raw <= 0xffff; ++raw)
    {
        const auto decoded = Kites::posit::decode(static_cast<uint16_t>(raw));
        if (decoded.nar) continue;
        EXPECT_EQ(Kites::posit::encode(decoded.negative, decoded.significand, decoded.scale), raw);
    }
}

TEST(Posit16Test, RoundsNearestTiesToEven)
{
    using Kites::posit::encode;
    // These are exact midpoints between adjacent values around 1.0.
    EXPECT_EQ(encode(false, 4097, -12), 0x4000); // even lower encoding
    EXPECT_EQ(encode(false, 4099, -12), 0x4002); // even upper encoding
}

TEST(Posit16Test, RoundsAtRegimeBoundariesUsingU1Threshold)
{
    using Kites::posit::encode;

    // At these regime-dominated intervals, U1 is not the arithmetic midpoint.
    EXPECT_EQ(encode(false, 1, -54), 0x0002);
    EXPECT_EQ(encode(false, 1, -49), 0x0004);
    EXPECT_EQ(encode(false, 1, 51), 0x7ffe);

    // The negative interval reverses the endpoints; two's-complement parity
    // makes the tie decision symmetric in magnitude.
    EXPECT_EQ(encode(true, 1, -54), 0xfffe);
    EXPECT_EQ(encode(true, 1, -49), 0xfffc);
    EXPECT_EQ(encode(true, 1, 51), 0x8002);
}

TEST(Posit16Test, EveryNonzeroUnderMinposProjectsToSignedMinpos)
{
    using Kites::posit::encode;
    EXPECT_EQ(encode(false, 1, -57), 0x0001);
    EXPECT_EQ(encode(true, 1, -57), 0xffff);
    EXPECT_EQ(encode(false, 1, -120), 0x0001);
    EXPECT_EQ(encode(true, 1, -120), 0xffff);
    EXPECT_EQ(encode(false, 0, -120), 0x0000);
}

TEST(Posit16Test, HandlesUnderflowOverflowAndNar)
{
    using Kites::posit::encode;
    EXPECT_EQ(encode(false, 1, -57), 0x0001); // nonzero underflow projects to minpos
    EXPECT_EQ(encode(false, 1, -56), 0x0001);
    EXPECT_EQ(encode(false, 1, 57), 0x7fff);
    EXPECT_EQ(encode(true, 1, 57), 0x8001);

    EXPECT_EQ(Kites::posit::add(0x8000, 0x4000), 0x8000);
    EXPECT_EQ(Kites::posit::mul(0x8000, 0x0000), 0x8000);
    EXPECT_EQ(Kites::posit::div(0x4000, 0x0000), 0x8000);
    EXPECT_EQ(Kites::posit::div(0xc000, 0x0000), 0x8000);
    EXPECT_EQ(Kites::posit::div(0x0000, 0x0000), 0x8000);
}

TEST(Posit16Test, ArithmeticUsesExactPositResults)
{
    using Kites::posit::add;
    using Kites::posit::div;
    using Kites::posit::mul;
    using Kites::posit::sub;

    EXPECT_EQ(add(0x4000, 0x4000), 0x4800); // 1 + 1 = 2
    EXPECT_EQ(sub(0x4800, 0x4000), 0x4000); // 2 - 1 = 1
    EXPECT_EQ(mul(0x4800, 0x4800), 0x5000); // 2 * 2 = 4
    EXPECT_EQ(div(0x4800, 0x4800), 0x4000); // 2 / 2 = 1
    EXPECT_EQ(add(0xc000, 0x4000), 0x0000); // -1 + 1 = 0
    EXPECT_EQ(mul(0xc000, 0x4800), 0xb800); // -1 * 2 = -2
    EXPECT_EQ(div(0x0001, 0x7fff), 0x0001); // under-minpos division projects to minpos
    EXPECT_EQ(add(0x7fff, 0x7fff), 0x7fff); // finite overflow saturates
}

TEST(Posit16Test, ArithmeticMatchesPreviouslyFailingStandardCases)
{
    using Kites::posit::mul;

    EXPECT_EQ(mul(0x7cbb, 0x7fbb), 0x7ffd);
    EXPECT_EQ(mul(0xfff4, 0xffd0), 0x0001);

    // Corresponding negative-side results.
    EXPECT_EQ(mul(0x8345, 0x7fbb), 0x8003);
    EXPECT_EQ(mul(0x000c, 0xffd0), 0xffff);
}

TEST(Posit16Test, StandardIntegerAndBinary32Conversions)
{
    using namespace Kites::posit;
    EXPECT_EQ(toInt32(0x4000), 1u);
    EXPECT_EQ(toInt32(0x4400), 2u); // 1.5 ties to even
    EXPECT_EQ(toInt32(0xbc00), static_cast<uint32_t>(-2)); // -1.5 ties to even
    EXPECT_EQ(toInt32(0x3800), 0u); // +0.5 ties to even
    EXPECT_EQ(toInt32(0xc800), 0u); // -0.5 ties to even
    EXPECT_EQ(toInt32(0x8000), 0x80000000u);
    EXPECT_EQ(toInt32(0x7fff), 0x80000000u); // outside int32 range
    EXPECT_EQ(toInt32(0x8001), 0x80000000u); // outside int32 range
    EXPECT_EQ(fromInt32(0x80000000u), 0x8000u);
    EXPECT_EQ(fromInt32(1), 0x4000u);
    EXPECT_EQ(fromFloat32Bits(0x00000000), 0x0000u);
    EXPECT_EQ(fromFloat32Bits(0x80000000), 0x0000u);
    EXPECT_EQ(fromFloat32Bits(0x7f800000), 0x8000u);
    EXPECT_EQ(fromFloat32Bits(0xff800000), 0x8000u);
    EXPECT_EQ(fromFloat32Bits(0x7fc00001), 0x8000u);
    EXPECT_EQ(fromFloat32Bits(0x00000001), 0x0001u);
    EXPECT_EQ(fromFloat32Bits(0x80000001), 0xffffu);
    EXPECT_EQ(fromFloat32Bits(0x7f7fffff), 0x7fffu);
    EXPECT_EQ(fromFloat32Bits(0xff7fffff), 0x8001u);
    EXPECT_EQ(toFloat32Bits(0), 0x00000000u);
    EXPECT_EQ(toFloat32Bits(0x8000), 0x7fc00000u);
    EXPECT_EQ(toFloat32Bits(0x4000), 0x3f800000u);
    EXPECT_EQ(toFloat32Bits(0xc000), 0xbf800000u);
}
