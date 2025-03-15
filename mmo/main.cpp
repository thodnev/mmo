#include "png_wrap.hpp"
#include "utils.hpp"
#include <cstdint>
#include <iostream>

//import types;
//import common;

uint8_t rndzero() {
    return 0;
}


int main(const int argc, char * const argv[])
{
    std::cout << "Test" << std::endl;

    // types::Stats<uint8_t> stat = {.STR = 2, .AGI = 5};
    // std::cout << stat << std::endl << "Size is: " << sizeof stat << std::endl;

    // common::RandGen<uint8_t> rnd(rndzero, 10);
    // for (auto i = 0; i < 100; i++) {
    //     std::cout << i << " : " << (uint32_t)rnd.random(0, 5) << std::endl;
    // }

    png_wrap::PngImage img("minimap/test4.png");
    std::cout << std::format("Image size: {} x {}\n", img.width, img.height);
    auto flat = utils::flatten_bits(img.data, img.width, img.height);
    std::cout << "FLAT SIZE: " << flat.size() << std::endl;
    std::cout << "\n=========\n" << utils::to_string(flat) << std::endl;
    // std::vector<char> tst = {'h', 'e', 'l', 'l', 'o'};
    // std::cout << "\n=========\n" << utils::to_string(tst) << std::endl;
    std::cout << "DONE" << std::endl;
    return 0;
}