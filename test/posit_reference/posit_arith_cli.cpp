#include "processor/posit.h"
#include <cstdint>
#include <iostream>

int main()
{
    char op;
    uint32_t lhs, rhs;
    while (std::cin >> op >> lhs >> rhs)
    {
        const auto a = static_cast<uint16_t>(lhs);
        const auto b = static_cast<uint16_t>(rhs);
        uint16_t result = 0;
        switch (op)
        {
        case 'A': result = Kites::posit::add(a, b); break;
        case 'S': result = Kites::posit::sub(a, b); break;
        case 'M': result = Kites::posit::mul(a, b); break;
        case 'D': result = Kites::posit::div(a, b); break;
        }
        std::cout << result << '\n';
    }
}
