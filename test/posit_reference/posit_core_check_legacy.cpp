#include "processor/posit.h"
#include <cassert>
#include <cstdint>

int main()
{
    using namespace Kites::posit;
    for (uint32_t raw = 0; raw <= 0xffff; ++raw)
    {
        const auto d = decode(static_cast<uint16_t>(raw));
        if (d.nar) continue;
        assert(encode(d.negative, d.significand, d.scale) == raw);
    }
    assert(encode(false, 4097, -12) == 0x4000);
    assert(encode(false, 4099, -12) == 0x4002);
    assert(encode(false, 1, 1) == 0x4800);
    assert(encode(false, 1, 2) == 0x5000);
    assert(encode(false, 1, -1) == 0x3800);
    assert(encode(true, 1, 0) == 0xc000);
    assert(encode(false, 1, -57) == 0x0000);
    assert(encode(false, 1, -56) == 0x0001);
    assert(encode(false, 1, 57) == 0x7fff);
    assert(encode(true, 1, 57) == 0x8001);
    assert(add(0x4000, 0x4000) == 0x4800);
    assert(sub(0x4800, 0x4000) == 0x4000);
    assert(mul(0x4800, 0x4800) == 0x5000);
    assert(div(0x4800, 0x4800) == 0x4000);
    assert(div(0x4000, 0x0000) == 0x8000);
    assert(div(0x0001, 0x7fff) == 0x0000);
    assert(add(0x7fff, 0x7fff) == 0x7fff);
    assert(mul(0xc000, 0x4800) == 0xb800);
    assert(decode(0x0001).scale == -56);
    assert(decode(0x7fff).scale == 56);
}
