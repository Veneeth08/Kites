#include "processor/posit.h"
#include <cstdint>
#include <iostream>

int main()
{
    bool negative;
    uint64_t significand;
    int scale;
    while (std::cin >> negative >> significand >> scale)
        std::cout << Kites::posit::encode(negative, significand, scale) << '\n';
}
