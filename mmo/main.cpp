#include <cstdint>
#include <iostream>

import types;
import common;

uint8_t rndzero() {
    return 0;
}


int main(const int argc, char * const argv[])
{
    std::cout << "Test" << std::endl;

    types::Stats<uint8_t> stat = {.STR = 2, .AGI = 5};
    std::cout << stat << std::endl << "Size is: " << sizeof stat << std::endl;

    // common::RandGen<uint8_t> rnd(rndzero, 10);
    // for (auto i = 0; i < 100; i++) {
    //     std::cout << i << " : " << (uint32_t)rnd.random(0, 5) << std::endl;
    // }
    return 0;
}