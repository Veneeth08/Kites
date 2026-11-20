#include "processor/posit.h"

#include <array>
#include <cstddef>

namespace Kites::posit
{
namespace
{
constexpr uint16_t NAR = 0x8000;
constexpr uint16_t MAX_POS = 0x7fff;

// A small fixed-width unsigned integer is enough for exact Posit16 operations.
// Eight base-2^32 limbs cover the largest intermediate in rational rounding.
struct Big
{
    std::array<uint32_t, 8> limb{};
};

Big fromU64(uint64_t value)
{
    Big result;
    result.limb[0] = static_cast<uint32_t>(value);
    result.limb[1] = static_cast<uint32_t>(value >> 32);
    return result;
}

bool isZero(const Big &value)
{
    for (uint32_t limb : value.limb)
        if (limb != 0) return false;
    return true;
}

int compare(const Big &lhs, const Big &rhs)
{
    for (size_t i = lhs.limb.size(); i-- > 0;)
    {
        if (lhs.limb[i] < rhs.limb[i]) return -1;
        if (lhs.limb[i] > rhs.limb[i]) return 1;
    }
    return 0;
}

Big addBig(const Big &lhs, const Big &rhs)
{
    Big result;
    uint64_t carry = 0;
    for (size_t i = 0; i < result.limb.size(); ++i)
    {
        const uint64_t sum = static_cast<uint64_t>(lhs.limb[i]) + rhs.limb[i] + carry;
        result.limb[i] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }
    return result;
}

// Requires lhs >= rhs.
Big subtractBig(const Big &lhs, const Big &rhs)
{
    Big result;
    uint64_t borrow = 0;
    for (size_t i = 0; i < result.limb.size(); ++i)
    {
        const uint64_t a = lhs.limb[i];
        const uint64_t b = static_cast<uint64_t>(rhs.limb[i]) + borrow;
        result.limb[i] = static_cast<uint32_t>(a - b);
        borrow = a < b;
    }
    return result;
}

Big shiftLeft(const Big &value, unsigned bits)
{
    Big result;
    if (bits >= result.limb.size() * 32) return result;
    const size_t words = bits / 32;
    const unsigned remainder = bits % 32;
    for (size_t i = 0; i + words < result.limb.size(); ++i)
    {
        const uint64_t shifted = static_cast<uint64_t>(value.limb[i]) << remainder;
        result.limb[i + words] |= static_cast<uint32_t>(shifted);
        if (remainder != 0 && i + words + 1 < result.limb.size())
            result.limb[i + words + 1] |= static_cast<uint32_t>(shifted >> 32);
    }
    return result;
}

Big multiply(const Big &lhs, const Big &rhs)
{
    Big result;
    for (size_t i = 0; i < lhs.limb.size(); ++i)
    {
        uint64_t carry = 0;
        for (size_t j = 0; j + i < result.limb.size(); ++j)
        {
            const size_t k = i + j;
            const uint64_t product = static_cast<uint64_t>(lhs.limb[i]) * rhs.limb[j] +
                                     result.limb[k] + carry;
            result.limb[k] = static_cast<uint32_t>(product);
            carry = product >> 32;
        }
    }
    return result;
}

Big negateBits(const Big &value)
{
    Big result;
    uint64_t carry = 1;
    for (size_t i = 0; i < result.limb.size(); ++i)
    {
        const uint64_t word = static_cast<uint64_t>(~value.limb[i]) + carry;
        result.limb[i] = static_cast<uint32_t>(word);
        carry = word >> 32;
    }
    return result;
}

bool signBit(const Big &value)
{
    return (value.limb.back() & 0x80000000u) != 0;
}

bool addMagnitude(const Big &lhs, const Big &rhs, Big &result)
{
    uint64_t carry = 0;
    for (size_t i = 0; i < result.limb.size(); ++i)
    {
        const uint64_t sum = static_cast<uint64_t>(lhs.limb[i]) + rhs.limb[i] + carry;
        result.limb[i] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }
    return carry != 0;
}

Big productUnits(uint16_t a, uint16_t b, bool &negative)
{
    const Decoded left = decode(a);
    const Decoded right = decode(b);
    negative = left.negative != right.negative;
    const uint32_t product = static_cast<uint32_t>(left.significand) * right.significand;
    const int shift = left.scale + right.scale + 112;
    // Posit16 scales are [-56, 56], so shift is [0, 224].
    return shiftLeft(fromU64(product), static_cast<unsigned>(shift));
}

Quire256 signedAddProduct(const Quire256 &q, uint16_t a, uint16_t b, bool subtract)
{
    if (isQuireNaR(q) || a == NAR || b == NAR) return quireNaR();

    const bool qNegative = signBit(Big{q.limbs});
    const Big qBits{q.limbs};
    const Big qMagnitude = qNegative ? negateBits(qBits) : qBits;

    bool productNegative = false;
    const Big pMagnitude = productUnits(a, b, productNegative);
    if (subtract && !isZero(pMagnitude)) productNegative = !productNegative;

    Big resultMagnitude;
    bool resultNegative = false;
    bool overflow = false;
    if (qNegative == productNegative)
    {
        overflow = addMagnitude(qMagnitude, pMagnitude, resultMagnitude);
        resultNegative = qNegative;
    }
    else if (compare(qMagnitude, pMagnitude) >= 0)
    {
        resultMagnitude = subtractBig(qMagnitude, pMagnitude);
        resultNegative = qNegative;
    }
    else
    {
        resultMagnitude = subtractBig(pMagnitude, qMagnitude);
        resultNegative = productNegative;
    }

    // The magnitude 2^255 is the reserved NaR encoding. Greater magnitudes
    // are signed overflow; neither may wrap into a finite Quire value.
    if (overflow || signBit(resultMagnitude))
        return quireNaR();
    Big resultBits = resultNegative && !isZero(resultMagnitude)
                         ? negateBits(resultMagnitude)
                         : resultMagnitude;
    return {resultBits.limb};
}

struct Fraction
{
    bool negative = false;
    Big numerator{};
    Big denominator = fromU64(1);
};

Fraction fromDyadic(bool negative, uint64_t significand, int scale)
{
    Fraction result;
    result.negative = negative;
    result.numerator = fromU64(significand);
    if (scale >= 0)
        result.numerator = shiftLeft(result.numerator, static_cast<unsigned>(scale));
    else
        result.denominator = shiftLeft(result.denominator, static_cast<unsigned>(-scale));
    return result;
}

Fraction fromBits(uint16_t bits)
{
    const Decoded decoded = decode(bits);
    return fromDyadic(decoded.negative, decoded.significand, decoded.scale);
}

Fraction addFractions(const Fraction &lhs, const Fraction &rhs)
{
    const Big left = multiply(lhs.numerator, rhs.denominator);
    const Big right = multiply(rhs.numerator, lhs.denominator);
    Fraction result;
    result.denominator = multiply(lhs.denominator, rhs.denominator);
    if (lhs.negative == rhs.negative)
    {
        result.negative = lhs.negative;
        result.numerator = addBig(left, right);
    }
    else if (compare(left, right) >= 0)
    {
        result.negative = lhs.negative;
        result.numerator = subtractBig(left, right);
    }
    else
    {
        result.negative = rhs.negative;
        result.numerator = subtractBig(right, left);
    }
    if (isZero(result.numerator)) result.negative = false;
    return result;
}

// Compares a nonnegative fraction with significand * 2^scale.
int compareWithDyadic(const Big &numerator, const Big &denominator,
                      uint64_t significand, int scale)
{
    Big dyadicNumerator = fromU64(significand);
    Big dyadicDenominator = fromU64(1);
    if (scale >= 0)
        dyadicNumerator = shiftLeft(dyadicNumerator, static_cast<unsigned>(scale));
    else
        dyadicDenominator = shiftLeft(dyadicDenominator, static_cast<unsigned>(-scale));
    return compare(multiply(numerator, dyadicDenominator),
                   multiply(dyadicNumerator, denominator));
}

Decoded decodePositiveCode(uint32_t bits, int precision)
{
    int cursor = precision - 2;
    const bool regimeBit = ((bits >> cursor) & 1) != 0;
    int run = 0;
    while (cursor >= 0 && (((bits >> cursor) & 1) == regimeBit))
    {
        ++run;
        --cursor;
    }

    const int regime = regimeBit ? run - 1 : -run;
    if (cursor >= 0) --cursor; // Skip the regime terminator.

    int exponent = 0;
    for (int i = 0; i < 2; ++i)
    {
        const unsigned exponentBit = cursor >= 0
                                         ? ((bits >> static_cast<unsigned>(cursor--)) & 1u)
                                         : 0u;
        exponent = (exponent << 1) | static_cast<int>(exponentBit);
    }

    uint16_t fraction = 0;
    int fractionBits = 0;
    while (cursor >= 0)
    {
        fraction = static_cast<uint16_t>((fraction << 1) | ((bits >> cursor--) & 1));
        ++fractionBits;
    }
    const uint16_t significand = static_cast<uint16_t>((1u << fractionBits) + fraction);
    return {false, significand, 4 * regime + exponent - fractionBits, false};
}

int compareWithPosit(const Fraction &value, uint16_t bits)
{
    const Decoded decoded = decode(bits);
    return compareWithDyadic(value.numerator, value.denominator,
                             decoded.significand, decoded.scale);
}

uint16_t roundFraction(Fraction value)
{
    if (isZero(value.numerator)) return 0;

    // The 2022 Posit Standard projects every nonzero value below minpos to
    // signed minpos; zero is reserved for an exactly zero result.
    if (compareWithPosit(value, 0x0001) < 0)
        return value.negative ? 0xffff : 0x0001;

    // Anything at or beyond maxpos is maxpos, including the exact endpoint.
    if (compareWithPosit(value, MAX_POS) >= 0)
        return value.negative ? static_cast<uint16_t>(~MAX_POS + 1) : MAX_POS;

    // Find adjacent positive encodings. The minpos check above guarantees
    // that low is nonzero and the maxpos check guarantees high is above x.
    uint32_t low = 1;
    uint32_t high = MAX_POS;
    while (low + 1 < high)
    {
        const uint32_t middle = (low + high) / 2;
        if (compareWithPosit(value, static_cast<uint16_t>(middle)) >= 0)
            low = middle;
        else
            high = middle;
    }
    if (compareWithPosit(value, static_cast<uint16_t>(low)) == 0)
    {
        const uint16_t exact = static_cast<uint16_t>(low);
        return value.negative ? static_cast<uint16_t>(~exact + 1) : exact;
    }

    // Section 4.1 defines the boundary using the (n+1)-bit posit made by
    // appending 1 to U. This is not always the arithmetic midpoint of the
    // adjacent n-bit values (notably in regime-dominated intervals).
    const uint32_t u1 = (low << 1) | 1u;
    const Decoded threshold = decodePositiveCode(u1, 17);
    const int thresholdComparison = compareWithDyadic(value.numerator, value.denominator,
                                                       threshold.significand, threshold.scale);
    uint16_t rounded;
    if (thresholdComparison < 0 || (thresholdComparison == 0 && (low & 1) == 0))
        rounded = static_cast<uint16_t>(low);
    else
        rounded = static_cast<uint16_t>(high);

    // Rounding the magnitude and then applying two's complement also gives
    // the Standard's negative-side decision: the negative interval reverses
    // the endpoints and the lower raw-code parity flips with that reversal.
    return value.negative ? static_cast<uint16_t>(~rounded + 1) : rounded;
}
} // namespace

Quire256 quireZero()
{
    return {};
}

Quire256 quireNaR()
{
    Quire256 result;
    result.limbs.back() = 0x80000000u;
    return result;
}

bool isQuireNaR(const Quire256 &value)
{
    if (value.limbs.back() != 0x80000000u) return false;
    for (size_t i = 0; i + 1 < value.limbs.size(); ++i)
        if (value.limbs[i] != 0) return false;
    return true;
}

Quire256 quireMulAdd(const Quire256 &q, uint16_t a, uint16_t b)
{
    return signedAddProduct(q, a, b, false);
}

Quire256 quireMulSub(const Quire256 &q, uint16_t a, uint16_t b)
{
    return signedAddProduct(q, a, b, true);
}

uint16_t quireToPosit(const Quire256 &value)
{
    if (isQuireNaR(value)) return NAR;

    const Big bits{value.limbs};
    const bool negative = signBit(bits);
    const Big magnitude = negative ? negateBits(bits) : bits;
    // Keep every subsequent Fraction cross-product within the existing
    // 256-bit Big capacity. |Q| >= 2^168 units is already at least maxpos.
    if (compare(magnitude, shiftLeft(fromU64(1), 168)) >= 0)
        return negative ? static_cast<uint16_t>(~MAX_POS + 1) : MAX_POS;
    Fraction exact;
    exact.negative = negative;
    exact.numerator = magnitude;
    exact.denominator = shiftLeft(fromU64(1), 112);
    return roundFraction(exact);
}

Decoded decode(uint16_t bits)
{
    if (bits == 0) return {};
    if (bits == NAR) return {.nar = true};

    const bool negative = (bits & 0x8000) != 0;
    const uint16_t magnitude = negative ? static_cast<uint16_t>(~bits + 1) : bits;
    Decoded result = decodePositiveCode(magnitude, 16);
    result.negative = negative;
    return result;
}

uint16_t encode(bool negative, uint64_t significand, int scale)
{
    if (significand == 0) return 0;
    if (scale > 56)
        return negative ? static_cast<uint16_t>(~MAX_POS + 1) : MAX_POS;
    // Even an input far below half-minpos projects to signed minpos under the
    // Posit Standard. The zero-significand case above remains exact zero.
    if (scale < -120) return negative ? 0xffff : 0x0001;
    return roundFraction(fromDyadic(negative, significand, scale));
}

uint16_t add(uint16_t lhs, uint16_t rhs)
{
    if (lhs == NAR || rhs == NAR) return NAR;
    return roundFraction(addFractions(fromBits(lhs), fromBits(rhs)));
}

uint16_t sub(uint16_t lhs, uint16_t rhs)
{
    if (lhs == NAR || rhs == NAR) return NAR;
    Fraction right = fromBits(rhs);
    if (!isZero(right.numerator)) right.negative = !right.negative;
    return roundFraction(addFractions(fromBits(lhs), right));
}

uint16_t mul(uint16_t lhs, uint16_t rhs)
{
    if (lhs == NAR || rhs == NAR) return NAR;
    const Fraction left = fromBits(lhs);
    const Fraction right = fromBits(rhs);
    Fraction result;
    result.negative = left.negative != right.negative;
    result.numerator = multiply(left.numerator, right.numerator);
    result.denominator = multiply(left.denominator, right.denominator);
    return roundFraction(result);
}

uint16_t div(uint16_t lhs, uint16_t rhs)
{
    if (lhs == NAR || rhs == NAR) return NAR;
    const Fraction left = fromBits(lhs);
    const Fraction right = fromBits(rhs);
    if (isZero(right.numerator)) return NAR;
    Fraction result;
    result.negative = left.negative != right.negative;
    result.numerator = multiply(left.numerator, right.denominator);
    result.denominator = multiply(left.denominator, right.numerator);
    return roundFraction(result);
}

uint32_t toInt32(uint16_t bits)
{
    const Decoded d = decode(bits);
    if (d.nar) return 0x80000000u;
    if (d.significand == 0) return 0;

    uint64_t magnitude = d.significand;
    if (d.scale >= 0)
    {
        if (d.scale >= 32 || magnitude > (uint64_t{1} << 31) >> d.scale)
            return 0x80000000u;
        magnitude <<= static_cast<unsigned>(d.scale);
    }
    else
    {
        const unsigned shift = static_cast<unsigned>(-d.scale);
        uint64_t whole = shift >= 64 ? 0 : magnitude >> shift;
        const uint64_t remainder = shift >= 64 ? magnitude :
                                   magnitude & ((uint64_t{1} << shift) - 1);
        const bool aboveHalf = shift < 64 &&
            remainder > (uint64_t{1} << (shift - 1));
        const bool tie = shift < 64 &&
            remainder == (uint64_t{1} << (shift - 1));
        if (aboveHalf || (tie && (whole & 1))) ++whole;
        magnitude = whole;
    }

    const uint64_t limit = d.negative ? uint64_t{1} << 31 : INT32_MAX;
    if (magnitude > limit) return 0x80000000u;
    const int64_t signedValue = d.negative ? -static_cast<int64_t>(magnitude)
                                           : static_cast<int64_t>(magnitude);
    return static_cast<uint32_t>(static_cast<int32_t>(signedValue));
}

uint16_t fromInt32(uint32_t bits)
{
    const bool negative = (bits >> 31) != 0;
    const uint64_t magnitude = negative ? static_cast<uint64_t>(~bits + 1u) : bits;
    return encode(negative, magnitude, 0);
}

uint16_t fromFloat32Bits(uint32_t bits)
{
    const bool negative = (bits >> 31) != 0;
    const uint32_t exponent = (bits >> 23) & 0xff;
    const uint32_t fraction = bits & 0x7fffff;
    if (exponent == 0xff) return NAR;
    if (exponent == 0 && fraction == 0) return 0;
    if (exponent == 0)
        return encode(negative, fraction, -149);
    return encode(negative, (uint32_t{1} << 23) | fraction,
                  static_cast<int>(exponent) - 127 - 23);
}

uint32_t toFloat32Bits(uint16_t bits)
{
    const Decoded d = decode(bits);
    if (d.nar) return 0x7fc00000u;
    if (d.significand == 0) return 0;

    uint32_t significand = d.significand;
    int top = 31;
    while (((significand >> top) & 1u) == 0u) --top;
    const int unbiased = top + d.scale;
    const uint32_t sign = d.negative ? 0x80000000u : 0u;
    // All nonzero Posit16 ES=2 values are normal and exactly representable in binary32.
    const uint32_t normalized = significand << (23 - top);
    return sign | (static_cast<uint32_t>(unbiased + 127) << 23) |
           (normalized & 0x7fffffu);
}
} // namespace Kites::posit
