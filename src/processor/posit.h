#pragma once

#include <cstdint>

namespace Kites::posit
{
/// Exact decoded form: (-1)^negative * significand * 2^scale.
/// Zero has significand 0; NaR is reported separately.
struct Decoded
{
    bool negative = false;
    uint16_t significand = 0;
    int scale = 0;
    bool nar = false;
};

/// Decode Posit16 (ES=2) without using host floating-point arithmetic.
Decoded decode(uint16_t bits);

/// Encode an exact integer significand times a power of two, using
/// round-to-nearest, ties-to-even. Values outside the finite range saturate.
uint16_t encode(bool negative, uint64_t significand, int scale);

uint16_t add(uint16_t lhs, uint16_t rhs);
uint16_t sub(uint16_t lhs, uint16_t rhs);
uint16_t mul(uint16_t lhs, uint16_t rhs);
uint16_t div(uint16_t lhs, uint16_t rhs);

/// Convert Posit16 to signed 32-bit using nearestInt (ties to even).
uint32_t toInt32(uint16_t bits);
/// Convert the two's-complement int32 bit pattern to Posit16.
uint16_t fromInt32(uint32_t bits);
/// Convert IEEE-754 binary32 bits to Posit16 using exact Section 4.1 rounding.
uint16_t fromFloat32Bits(uint32_t bits);
/// Convert Posit16 to binary32 bits; finite inputs are exact.
uint32_t toFloat32Bits(uint16_t bits);
} // namespace Kites::posit
