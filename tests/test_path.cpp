#include <array>
#include <bitset>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <iostream>

// import types;
// [[gnu::packed]]

// Coordinate delta constraint: int -1, 0 or 1
template <typename T>
concept pe_coord_constraint = (
    std::is_integral<T>::value && std::is_signed<T>::value
    && (-1 <= T{} && T{} <= 1)
);


struct PathEntry {
public:
    static constexpr size_t bitlen = 3;
private:
    using delta_t = int8_t;     // For coordinate delta, e.g. (-1, 1)
    using pair_t = std::tuple<delta_t, delta_t>;
    using bset_t = std::bitset<bitlen>;
    //  b2 | b1 | b0
    //  r  | q  | p
    bset_t data;

    // LUT for converting bits to (dX, dY) coordinate deltas
    // Coordinates are indexed as:
    // index = ((dY + 1) * 3 + dX + 5) % 9
    // except (0, 0) -- produces 8 -- forbidden combination
    static constexpr std::array<pair_t, (1 << bitlen)>
        _table_data_to_coords = {
            //                          rqp      (dX, dY)
            pair_t{ 1,  0},  // [0] = 0b000  ->  ( 1,  0)
            pair_t{-1,  1},  // [1] = 0b001  ->  (-1,  1)
            pair_t{ 0,  1},  // [2] = 0b010  ->  ( 0,  1)
            pair_t{ 1,  1},  // [3] = 0b011  ->  ( 1,  1)
            pair_t{-1, -1},  // [4] = 0b100  ->  (-1, -1)
            pair_t{ 0, -1},  // [5] = 0b101  ->  ( 0, -1)
            pair_t{ 1, -1},  // [6] = 0b110  ->  ( 1, -1)
            pair_t{-1,  0}   // [7] = 0b111  ->  (-1,  0)
    };

public:
    PathEntry() = delete;       // prevent default constructor

    constexpr PathEntry(const bset_t bits) noexcept
              : data(bits) {}

    constexpr PathEntry(const auto dX, const auto dY)
              : data(dxdy_to_bitset(dX, dY)) {}

    constexpr auto to_coords() noexcept
    {
        return _table_data_to_coords[data.to_ulong()];
    }

    constexpr auto to_bitset() noexcept
    {
        return data;
    }

    template<typename T>
    requires pe_coord_constraint<T>
    static constexpr bset_t dxdy_to_bitset(const T dX, const T dY)
    {
        [[unlikely]] if (dX == 0 && dY == 0) {
            throw std::out_of_range("(dX, dY) cannot be (0, 0)");
        }
        // Coordinates are indexed as:
        // data = index = ((dY + 1) * 3 + dX + 5) % 9
        // except (0, 0) -- produces 8 -- forbidden combination
        // This way no inverse LUT is needed, just compute the index
        return ( ((int)dY + 1) * 3 + ((int)dX + 5) ) % 9;
    }
};

#include <format>
#include <vector>
int main(const int argc, char * const argv[])
{
    std::cout << "Test path\n";

    auto tst = PathEntry(0b111);
    std::cout << "SIZE: " << sizeof(tst) << "\n";
    for (unsigned int bits = 0; bits < 8; bits++) {
        auto ent = PathEntry(bits);
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
        auto ent = PathEntry(dx, dy);
        auto bset = ent.to_bitset();

        auto msg = std::format(
            "[{:>2}, {:>2}]  {}  {}",
            dx, dy, bset.to_string(), bset.to_ulong()
        );
        std::cout << msg << std::endl;
    }

    std::cout << "\nTrying forbidden combination (0,0)\n";
    try {
        auto forb1 = PathEntry(0, 0);
        //auto forb2 = PathEntry::dxdy_to_bitset(0, 0);
    } catch (std::exception exc) {
        std::cout << "caught " << exc.what() << std::endl;
    }
    std::cout << "\nThat's all\n";

    return 0;
}