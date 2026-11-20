#include "processor/posit.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

int main()
{
    char operation;
    while (std::cin >> operation)
    {
        if (operation == 'R')
        {
            Kites::posit::Quire256 q;
            for (int i = 7; i >= 0; --i)
            {
                uint32_t word;
                std::cin >> std::hex >> word >> std::dec;
                q.limbs[static_cast<size_t>(i)] = word;
            }
            std::cout << Kites::posit::quireToPosit(q) << '\n';
            continue;
        }

        Kites::posit::Quire256 q;
        for (int i = 7; i >= 0; --i)
        {
            uint32_t word;
            std::cin >> std::hex >> word >> std::dec;
            q.limbs[static_cast<size_t>(i)] = word;
        }
        uint32_t a, b;
        std::cin >> std::hex >> a >> b >> std::dec;
        q = operation == 'A'
                ? Kites::posit::quireMulAdd(q, static_cast<uint16_t>(a), static_cast<uint16_t>(b))
                : Kites::posit::quireMulSub(q, static_cast<uint16_t>(a), static_cast<uint16_t>(b));
        for (int i = 7; i >= 0; --i)
            std::cout << std::hex << std::setw(8) << std::setfill('0')
                      << q.limbs[static_cast<size_t>(i)];
        std::cout << std::dec << '\n';
    }
}
