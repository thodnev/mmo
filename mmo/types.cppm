module;     // Global module fragment
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

export module types;
export namespace types {

template <typename T = uint8_t>
struct Stats
{
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

    operator std::vector<T>() const {
        return this->to_vector();
    }

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