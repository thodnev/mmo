#include "png_wrap.hpp"
#include "utils.hpp"

#include <cstdint>
#include <iostream>

import common;
//import rnd;
//import types;


int main(const int argc, char * const argv[])
{
    std::cout << "Test" << std::endl;

    // types::Stats<uint8_t> stat = {.STR = 2, .AGI = 5};
    // std::cout << stat << std::endl << "Size is: " << sizeof stat << std::endl;

    // png_wrap::PngImage img("minimap/test4.png");
    // std::cout << std::format("Image size: {} x {}\n", img.width, img.height);
    // auto flat = utils::flatten_bits(img.data, img.width, img.height);
    // std::cout << "FLAT SIZE: " << flat.size() << std::endl;
    // std::cout << "\n=========\n" << utils::to_string(flat) << std::endl;

    // std::cout << "Flat 1s: " << utils::count_bits(flat) << std::endl;
    // std::cout << "DONE" << std::endl;

    // common::BinMask mask("minimap/test4.png");
    // std::cout << std::format("Mask size: {} x {}\n", mask.width, mask.height);
    // std::cout << std::format("  set bits: {}\n", mask.num_set_bits());

    // std::cout << "NON-ZERO POINTS:\n";
    // for (size_t bit = 0; bit < mask.num_set_bits(); bit++) {
    //     auto [x, y] = mask.get_coords_nonzero(bit);
    //     std::cout << "(" << x << ", " << y << ")" << ",\t";
    // }
    // std::cout << std::endl;

    // std::cout << "IMAGE DATA:\n";
    // for (auto row = 0; row < mask.height; row++) {
    //     for (auto col = 0; col < mask.width; col++) {
    //         auto el = mask.get_value(col, row);
    //         std::cout << (el ? "##" : "__");
    //     }
    //     std::cout << std::endl;
    // }
    
    //std::cout << "out of bonds: " << mask.get_value(33, 37) << std::endl;
    return 0;
}