#include <stdexcept>
#include <type_traits>
#include <vector>

import types;
using namespace types;

#include <iostream>
#include <bitset>
#include <exception>
#include <format>

int main(const int argc, char * const argv[])
{
    std::cout << "Test path\n";

    auto tst = types::PathEntry(0b111);
    std::cout << "SIZE: " << sizeof(tst) << "\n";
    for (unsigned int bits = 0; bits < 8; bits++) {
        auto ent = types::PathEntry(bits);
        auto [dx, dy] = ent.to_coords();
        std::cout << bits << "  " << ent.to_bitset();
        std::cout << "  (" <<  (int)dx << ", " << (int)dy << ")\n";
    }

    std::cout << "\nTrying inverse lookup by coordinates\n";
    using cpair_t = std::pair<int, int>;
    std::vector<cpair_t> deltas = {
        cpair_t{ 1,  0},
        cpair_t{-1,  1},
        cpair_t{ 0,  1},
        cpair_t{ 1,  1},
        cpair_t{-1, -1},
        cpair_t{ 0, -1},
        cpair_t{ 1, -1},
        cpair_t{-1,  0}
    };

    for (auto [dx, dy]: deltas) {
        //auto bset = PathEntry::dxdy_to_bitset(dx, dy);
        auto ent = types::PathEntry(dx, dy);
        auto bset = ent.to_bitset();

        auto msg = std::format(
            "[{:>2}, {:>2}]  {}  {}",
            dx, dy, bset.to_string(), bset.to_ulong()
        );
        std::cout << msg << std::endl;
    }

    std::cout << "\nTrying forbidden combination (0,0)\n";
    try {
        auto forb1 = types::PathEntry(0, 0);
        //auto forb2 = PathEntry::dxdy_to_bitset(0, 0);
    } catch (std::exception exc) {
        std::cout << "caught " << exc.what() << std::endl;
    }
    std::cout << "\nThat's all\n";

    // Path
    Path pth = {
        PathEntry{-1, -1},          // element [0] stored at tail
        PathEntry{1, 0},
        PathEntry{0, 1},
        PathEntry{1, 1}
    };
    
    std::cout << "Elements in reverse order: ";
    for (const auto& el : pth) {
        std::cout << el << " ";
    }
    std::cout << std::endl;

    std::cout << "Access via operator[]: ";
    for (size_t i = 0; i < pth.size(); ++i) {
        std::cout << pth[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "LAST stored:" << pth.pop_next() << "\n";

    return 0;
}