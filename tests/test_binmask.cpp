#include <iostream>

import common;

int main(const int argc, char * const argv[])
{
    std::cout << "Test binmask" << std::endl;

    common::BinMask mask("minimap/test4.png");
    std::cout << std::format("Mask size: {} x {}\n", mask.width, mask.height);
    std::cout << std::format("  set bits: {}\n", mask.num_set_bits());

    common::IndexedBinMask imask("minimap/test4.png");
    std::cout << std::format("IMask size: {} x {}\n", imask.width, imask.height);
    std::cout << std::format("  set bits: {}\n", imask.num_set_bits());

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
    common::BinMask minimap("minimap/minimap.png");
    std::cout << "MINIMAP black: " << minimap.get_value(72, 147) << std::endl;
    std::cout << "MINIMAP white: " << minimap.get_value(85, 30) << std::endl;

    // std::cout << "out of bonds: " << mask.get_value(33, 37) << std::endl;

return 0;
}