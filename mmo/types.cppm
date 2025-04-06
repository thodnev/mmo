module;     // Global module fragment
#include <array>
#include <bitset>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <limits>
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

// Core types

/// Represents one coordinate: X or Y.
/// This should be a signed integer as:
///   - it simplifies math and boundary checks;
///   - our map uses center at (0, 0) so coords can be negative anyway.
using axis_t = int16_t;

/// Relative coordinate inside bounding box.
/// Coordinates inside bbox are non-negative. But to simplify math checks
/// and avoid dealing with integers overflows, it is better to have it signed.
using bbox_t = axis_t;

/// Used to represent distance between two points
/// Octile distance metric, as implemented in project, returns maximum value
/// of input_max * ceil[(128 + 53) / 2] = 91 * input_max
/// This must not be huge to save memory in vector tables (of dist_t x visited size)
using dist_t = uint32_t;

static_assert(std::numeric_limits<dist_t>::max()
              >= ( std::numeric_limits<axis_t>::max() * (128 + 53) ) / 2,
              "dist_t is not wide enough to hold any axis_t distance");


/// Enum classes are strongly typed. We use them to distinguish between
/// two coordinate types, absolute Coord and relative RelCoord
/// defined below. Both implemented as CoordBase, but should be made
/// incompatible without explicit cast.
enum class CoordTag_Absolute {};
enum class CoordTag_Relative {};

/// Coordinate pair (X, Y) - base for two concrete pair types below
/// This should be used directly without subclassing
template <typename T, typename Tag>
struct CoordBase {
    using type = T;
    using kind = Tag;
    T x;
    T y;

    [[gnu::hot, gnu::always_inline]]
    constexpr inline bool is_zero() const noexcept
    {
        return !x && !y;
    }

    [[gnu::hot, gnu::always_inline]]
    constexpr inline bool operator==(const CoordBase<T, Tag> &other) const noexcept
    {
        return x == other.x && y == other.y;
    }

    /// utility for nicer output and debugging experience
    operator std::string() const noexcept
    {
        // use (x, y) for absolute and [x, y] for relative coords
        return std::format(
            std::is_same<Tag, CoordTag_Absolute>::value ?
            "({}, {})" : "[{}, {}]", x, y);
    }

    friend std::ostream& operator<<(std::ostream &out, const CoordBase<T, Tag> &coord)
    {
        out << static_cast<std::string>(coord);
        return out;
    }
};

/// Coord represents *absolute* coordinates on the whole map
/// See RelCoord below for relative coordinate type
using Coord = CoordBase<axis_t, CoordTag_Absolute>;
static_assert(sizeof(Coord) == 2 * sizeof(axis_t),
              "Coord struct not packed efficiently");

/// RelCoord represents coordinates *relative* to some basic point
/// (normally inside a bbox map region).
/// This way, even though Coord and RelCoord may use the same basic types inside,
/// they mean different things and should be made explicitly distinct.
/// Compiler should complain if the two are mixed together without explicit cast.
using RelCoord = CoordBase<bbox_t, CoordTag_Relative>;
static_assert(sizeof(RelCoord) == 2 * sizeof(bbox_t),
              "RelCoord struct not packed efficiently");


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
public:
    using pair_t = std::tuple<delta_t, delta_t>;
    using table_arr_t = std::array<pair_t, (1 << bitlen)>;

    /// LUT for converting bits to (dX, dY) coordinate deltas
    /// Coordinates are indexed as:
    /// index = ((dY + 1) * 3 + dX + 5) % 9
    /// except (0, 0) -- produces 8 -- forbidden combination
    static constexpr const table_arr_t
        _steps_table = {
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
private:
    using bset_t = std::bitset<bitlen>;
    //  b2 | b1 | b0
    //  r  | q  | p
    uint8_t data : bitlen;  ///< use bitfield, std::bitset uses 64 bits

    template<typename T>
    requires pe_coord_constraint<T>
    static constexpr auto _dxdy_to_index(const T dX, const T dY) noexcept
    {
        // Coordinates are indexed as:
        // data = index = ((dY + 1) * 3 + dX + 5) % 9
        // except (0, 0) -- produces 8 -- forbidden combination
        // This way no inverse LUT is needed, just compute the index
        return ( ((int)dY + 1) * 3 + ((int)dX + 5) ) % 9;
    }

public:
    PathEntry() = delete;       ///< prevent default constructor

    constexpr PathEntry(const bset_t bits) noexcept
                : data(bits.to_ulong()) {}

    constexpr PathEntry(const auto dX, const auto dY)
                : data(dxdy_to_bitset(dX, dY).to_ulong()) {}

    constexpr auto to_coords() const noexcept
    {
        return _steps_table[data];
    }

    //static constexpr const table_arr_t &get_steps_table() { return _steps_table; }

    constexpr bset_t to_bitset() const noexcept
    {
        return bset_t(data);
    }

    template<typename T>
    requires pe_coord_constraint<T>
    static bset_t dxdy_to_bitset(const T dX, const T dY)
    {
        // constexpr bool is_valid = (dX >= -1 && dY >= -1 && dX <= 1 && dY <= 1
        //                            && !(dX == 0 && dY == 0));
        // static_assert(is_valid, "(dX, dY) must be in range [-1, 1] and cannot be (0, 0)");
        
        [[unlikely]] if (dX < -1 || dY < -1 || dX > 1 || dY > 1
                         || (dX == 0 && dY == 0)) {
            throw std::out_of_range(
                "(dX, dY) must be in range [-1, 1] and cannot be (0, 0)");
        }

        return bset_t(_dxdy_to_index(dX, dY));
    }

    operator std::string() const {
        const auto [dX, dY] = to_coords();
        return std::format("PE({:>2}, {:>2})", dX, dY);
    }

    constexpr friend std::ostream& operator<<(std::ostream &os, const PathEntry &pe)
    {
        return os << static_cast<std::string>(pe);
    }
};


// @TODO: change inheritance to composition to ensure there will be no surprises
//
/// Path works as std::vector containing PathEntry, but internally
/// stores them in reverse order to ensure cheap pops from the head.
/// PEs must be located from last to first, as std::vector is cheap to pop
/// from the tail, and expensive to pop from the head.
class Path : public std::vector<PathEntry> {
private:
    using vec_t = std::vector<PathEntry>;

public:
    // constexpr coord_t from_point(const coord_t start) noexcept
    // {
    //     // ...
    //     return {0, 0};
    // }

    // Custom constructor to initialize in reverse order
    Path(std::initializer_list<PathEntry> items) 
        : vec_t(std::rbegin(items),
                std::rend(items)) {}
    //Path(const vec_t &vec) : vec_t(vec.rbegin(), vec.rend()) {}
        
    template<typename inp_iter>
    Path(inp_iter first, inp_iter last)
        : vec_t(std::reverse_iterator(last),
                std::reverse_iterator(first)) {}

    // Override operator[] to access elements in reverse order
    constexpr const PathEntry& operator[](size_t index) const {
        return vec_t::operator[](this->size() - 1 - index);
    }

    // Provide a reverse iterator as default
    constexpr auto begin() const { return vec_t::rbegin(); }
    constexpr auto end() const { return vec_t::rend(); }
    constexpr auto rbegin() const { return vec_t::begin(); }
    constexpr auto rend()const { return vec_t::end(); }

    constexpr auto pop_next()
    {
        if (this->empty()) {
            throw std::out_of_range("Cannot pop from empty Path");
        }
        auto el = this->back();     // get element
        this->pop_back();           // remove from the vector
        return el;
    }

    constexpr auto pop_delta() noexcept
    {
        try {
            auto el = this->pop_next();
            return el.to_coords();
        } catch (const std::out_of_range &exc) {
            return PathEntry::pair_t{0, 0};
        } 
    }
};

}       // namespace types
