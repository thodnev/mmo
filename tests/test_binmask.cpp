#include <iostream>

import common;

int main(const int argc, char * const argv[])
{
    std::cout << "Test binmask" << std::endl;

    using std::cerr;

    common::BinMask mask("minimap/test4.png");
    cerr << std::format("Mask size: {} x {}\n", mask.width, mask.height);
    cerr << std::format("  set bits: {}\n", mask.count_set_bits());

    common::IndexedBinMask imask("minimap/test4.png");
    cerr << std::format("IMask size: {} x {}\n", imask.width, imask.height);
    cerr << std::format("  set bits: {}\n", imask.count_set_bits());

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

    cerr << "Indexing map[x][y]\n";
    cerr << "MINIMAP black: " << minimap[72][147] << std::endl;
    cerr << "MINIMAP white: " << minimap[85][30] << std::endl;

    cerr << "Setting values map[x][y] to black\n";
    minimap[85][30] = 0; minimap[72][147] = 0;
    //minimap.set_value(72, 147, 0); minimap.set_value(85, 30, 0); 
    cerr << " (was black): " << minimap[72][147] << std::endl;
    cerr << " (was white): " << minimap[85][30] << std::endl;

    cerr << "Chain-setting values map[x][y] to 1\n";
    //minimap.set_value(72, 147, 1); minimap.set_value(85, 30, 1);
    minimap[85][30] = minimap[72][147] = 1;
    cerr << " (was black): " << minimap[72][147] << std::endl;
    cerr << " (was white): " << minimap[85][30] << std::endl;

    // std::cout << "out of bonds: " << mask.get_value(33, 37) << std::endl;

return 0;
}