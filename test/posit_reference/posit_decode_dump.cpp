#include "processor/posit.h"
#include <cstdint>
#include <iostream>

int main()
{
    for (uint32_t raw = 0; raw <= 0xffff; ++raw)
    {
        const auto d = Kites::posit::decode(static_cast<uint16_t>(raw));
        std::cout << raw << ' ' << d.nar << ' ' << d.negative << ' '
                  << d.significand << ' ' << d.scale << '\n';
    }
}
