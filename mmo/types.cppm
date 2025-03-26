module;     // Global module fragment
#include <array>
#include <bitset>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


export module types;

/// Coordinate delta constraint
/// Here we check that the type is integer and CAN HOLD values from -1 to 1.
/// (does not mean it holds, this why we need runtime check as well)
template <typename T>
concept pe_coord_constraint = (
    std::is_integral<T>::value && std::is_signed<T>::value
    && (-1 <= T{} && T{} <= 1)
);


export namespace types {

template <typename T = uint8_t>
struct Stats {
    T STR = 0;       // Strength
    T INT = 0;       // Intelligence
    T DEX = 0;       // Dexterity

    T CON = 0;       // Constitution
    T WIS = 0;       // Wisdom
    T AGI = 0;       // Agility

    T LUK = 0;       // Luck
    T INS = 0;       // Insanity

    std::vector<T> to_vector() const {
        return {
            this->STR, this->INT, this->DEX, this->CON,
            this->WIS, this->AGI, this->LUK, this->INS
        };
    }

    operator std::vector<T>() const { return this->to_vector(); }

    std::vector<std::pair<std::string, T>> to_pairs() const {
        const std::string names[] = { "STR", "INT", "DEX", "CON", "WIS", "AGI", "LUK", "INS" };
        auto vals = this->to_vector();

        decltype(this->to_pairs()) res;
        for (auto i = 0; i < vals.size(); i++) {
            res.emplace_back(names[i], vals[i]);
        }
        return res;
        // return {
        //     // __STR_PAIR(STR, this),
        //     // __STR_PAIR(INT, this),
        //     {"STR", this->STR},
        //     {"INT", this->INT},
        //     {"DEX", this->DEX},
        //     {"CON", this->CON},
        //     {"WIS", this->WIS},
        //     {"AGI", this->AGI},
        //     {"LUK", this->LUK},
        //     {"INS", this->INS}
        // };
    }

    std::string to_string(const std::string sep = ", ") const {
        std::string res;

        auto vec = this->to_pairs();
        for (auto it_pair = vec.begin(); it_pair != vec.end(); ) {
            res += it_pair->first + ": " + std::to_string(it_pair->second);
            if (++it_pair == vec.end()) {
                break;          // Avoid adding after the last string
            }
            res += sep;
        }

        return res;
    }

    operator std::string() const {
        return this->to_string();
    }

    friend std::ostream & operator<<(std::ostream &os, const Stats &stats) {
        return os << stats.to_string();
    }
};


struct PathEntry {
    public:
        static constexpr size_t bitlen = 3;
    private:
        using delta_t = int8_t;     // For coordinate delta, e.g. (-1, 1)
        using pair_t = std::tuple<delta_t, delta_t>;
        using bset_t = std::bitset<bitlen>;
        //  b2 | b1 | b0
        //  r  | q  | p
        uint8_t data : bitlen;  ///< use bitfield, std::bitset uses 64 bits
    
        /// LUT for converting bits to (dX, dY) coordinate deltas
        /// Coordinates are indexed as:
        /// index = ((dY + 1) * 3 + dX + 5) % 9
        /// except (0, 0) -- produces 8 -- forbidden combination
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
        PathEntry() = delete;       ///< prevent default constructor
    
        constexpr PathEntry(const bset_t bits) noexcept
                  : data(bits.to_ulong()) {}
    
        constexpr PathEntry(const auto dX, const auto dY)
                  : data(dxdy_to_bitset(dX, dY).to_ulong()) {}
    
        constexpr auto to_coords() noexcept
        {
            return _table_data_to_coords[data];
        }
    
        constexpr bset_t to_bitset() noexcept
        {
            return bset_t(data);
        }
    
        template<typename T>
        requires pe_coord_constraint<T>
        static constexpr bset_t dxdy_to_bitset(const T dX, const T dY)
        {
            [[unlikely]] if (
                    dX < -1 || dY < -1 || dX > 1 || dY > 1
                    || (dX == 0 && dY == 0)) {
                throw std::out_of_range(
                    "(dX, dY) must be in range [-1, 1] and cannot be (0, 0)"
                );
            }
            // Coordinates are indexed as:
            // data = index = ((dY + 1) * 3 + dX + 5) % 9
            // except (0, 0) -- produces 8 -- forbidden combination
            // This way no inverse LUT is needed, just compute the index
            return ( ((int)dY + 1) * 3 + ((int)dX + 5) ) % 9;
        }
};


template<typename T>
struct Coord {
    T x;
    T y;

    Coord(T x, T y) : x(x), y(y) {}
};
}       // namespace types


// (!) overloading std:: namespace is undefined behavior
//     but we need this for structured binding to work
export namespace std {
// Specialization of std::tuple_size
// Declares that Point<T> behaves like a tuple with 2 elements (x and y).
template <typename T>
struct tuple_size<types::Coord<T>> : std::integral_constant<std::size_t, 2> {};

// Specialization of std::tuple_element
// Defines the type of each element in Point<T>, making Point behave like a tuple.

template <typename T, std::size_t I>
struct tuple_element<I, types::Coord<T>> {
    // or .y, both are the same type
    using type = decltype(std::declval<types::Coord<T>>().x);
};

// Overload of std::get
// Provides a way to access elements using std::get<I>(point)
template <typename T, std::size_t I>
constexpr auto get(const types::Coord<T> &obj) -> decltype(auto)
{
    if constexpr (I == 0) {
        return obj.x;
    } else if constexpr (I == 1) {
        return obj.y;
    }
}
};   // namespace std