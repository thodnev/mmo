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
public:
    using pair_t = std::tuple<delta_t, delta_t>;
    using table_arr_t = std::array<pair_t, (1 << bitlen)>;
private:
    using bset_t = std::bitset<bitlen>;
    //  b2 | b1 | b0
    //  r  | q  | p
    uint8_t data : bitlen;  ///< use bitfield, std::bitset uses 64 bits

    /// LUT for converting bits to (dX, dY) coordinate deltas
    /// Coordinates are indexed as:
    /// index = ((dY + 1) * 3 + dX + 5) % 9
    /// except (0, 0) -- produces 8 -- forbidden combination
    static constexpr const table_arr_t
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
        return _table_data_to_coords[data];
    }

    static constexpr const table_arr_t &get_steps_table() { return _table_data_to_coords; }

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


template<typename T = int16_t>
struct [[gnu::packed]] Coord {
    T x;
    T y;

    Coord(const T x, const T y) : x(x), y(y) {}
    
    // TODO: fix conversions
    template<typename P>
    Coord(const std::pair<P, P> &pair) : Coord(pair.first, pair.second) {}

    constexpr size_t hash() const noexcept
    {
        // std::hash-like must return size_t,
        // for ints it usually returns the numbers itself
        // That is why, rotate by half of size_t while combining them
        auto hx = std::hash<decltype(x)>{}(x);
        auto hy = std::hash<decltype(y)>{}(y);
        // rotate by half
        constexpr const auto rot = sizeof(size_t) * 4;
        return ((hx << rot) | (hx >> rot)) ^ hy;
    }

    template<typename U = T>
    bool operator==(const Coord<U> &other) const noexcept {
        return (x == other.x) && (y == other.y);
    }

    operator std::string() const noexcept
    {
        return std::format("({}, {})", x, y);
    }

    /// Distance metric based on octile distance
    /// Octile distance is computed as:
    /// dst = max(Δx, Δy) + (√2​−1)⋅min(Δx,Δy)
    /// We approximate with integer arithmetic as:
    /// dst*128 = 128 * max(Δx, Δy) + 53 * min(Δx,Δy)
    /// Should give us 0.03% error
    constexpr unsigned long dist_metric(const Coord &other) const noexcept
    {
        unsigned long dx = std::abs((long)this->x - (long)other.x);
        unsigned long dy = std::abs((long)this->y - (long)other.y);

        auto dst = 128 * std::max(dx, dy) + 53 * std::min(dx, dy);
        // auto dst = dx + dy;
        return dst;
    }
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

template<typename T, std::size_t I>
struct tuple_element<I, types::Coord<T>> {
    // or .y, both are the same type
    using type = decltype(std::declval<types::Coord<T>>().x);
};

// Overload of std::get
// Provides a way to access elements using std::get<I>(point)
template<typename T, std::size_t I>
constexpr auto get(const types::Coord<T> &obj) -> decltype(auto)
{
    if constexpr (I == 0) {
        return obj.x;
    } else if constexpr (I == 1) {
        return obj.y;
    }
}

// Custom specialization of std::hash
template<typename T>
struct hash<types::Coord<T>>
{
    size_t operator()(const types::Coord<T> &coord) const noexcept
    {
        return coord.hash();    // delegate to own method
    }
};
};   // namespace std