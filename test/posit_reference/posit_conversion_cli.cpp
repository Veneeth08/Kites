#include "processor/posit.h"

#include <cstdint>
#include <iostream>

int main()
{
    char operation;
    uint32_t bits;
    while (std::cin >> operation >> std::hex >> bits >> std::dec)
    {
        if (operation == 'F')
            std::cout << Kites::posit::fromFloat32Bits(bits) << '\n';
        else if (operation == 'I')
            std::cout << Kites::posit::fromInt32(bits) << '\n';
        else if (operation == 'P')
            std::cout << Kites::posit::toInt32(static_cast<uint16_t>(bits)) << ' '
                      << Kites::posit::toFloat32Bits(static_cast<uint16_t>(bits)) << '\n';
    }
}
