module;
#include <array>
#include <concepts>
#include <cstdint>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>

import err;

export module entity;

// @TODO: these should probably go to other module

// @TODO: find a better name for it
/// Represents data serialization result
using packed_t = std::vector<uint8_t>;

/// Constraint for a class to be considered Packabe (serializable)
template <typename T>
concept Packable = requires(T a, const packed_t &data) {
    /// Must have instance method to_packed() returning packed_t
    { a.to_packed() } -> std::same_as<packed_t>;

    /// Must have class method from_packed(packed_t &) returning an object instance 
    { T::from_packed(data) } -> std::same_as<T>;
};

export namespace entity {
// Entity (Character, Monster, NPC) related data structures



template <typename T>
struct StatsCommon {
    using type = T;  ///< Make available in subclasses as simple alias
    static constexpr size_t size = 8;   ///< Number of elements to simplify access
    static constexpr std::array<std::string, size> field_names = {
        "STR", "INT", "DEX", "CON", "WIS", "AGI", "LUK", "INS"};

    T STR = 0;       ///< Strength
    T INT = 0;       ///< Intelligence
    T DEX = 0;       ///< Dexterity

    T CON = 0;       ///< Constitution
    T WIS = 0;       ///< Wisdom
    T AGI = 0;       ///< Agility

    T LUK = 0;       ///< Luck
    T INS = 0;       ///< Insanity
    
    constexpr auto to_tuple() const noexcept
    {
        return std::make_tuple(STR, INT, DEX, CON, WIS, AGI, LUK, INS);
    }

    constexpr auto to_pairs() const noexcept
    {
        // make_named_tuple(std::make_index_sequence<size>{});

        constexpr auto values = this->to_tuple();

        auto make_pairs = [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
            return std::make_tuple(
                std::make_pair(field_names[Is], std::get<Is>(values))...
            );
        };
        
        return make_pairs(std::make_index_sequence<size>{});
    }

    constexpr const std::string to_string(const std::string &sep = ", ") const noexcept
    {
        std::ostringstream out;
        out << "@FIXME" << sep;
        return out.str();
    }

    operator std::string() const noexcept
    {
        return to_string();
    }

    friend std::ostream& operator<<(std::ostream &out, const StatsCommon &obj)
    {
        out << static_cast<std::string>(obj);
        return out;
    }
};


struct BaseStats : public StatsCommon<uint8_t> {
    constexpr packed_t to_packed() const noexcept
    {
        packed_t vec = {STR, INT, DEX, CON, WIS, AGI, LUK, INS};
        return vec;
    }

    // constexpr static BaseStats from_packed(const packed_t &data)
    // {
    //     if (size != data.size()) [[unlikely]] {
    //         throw err::ValueError(:REPLACEME);
    //     }

    //     BaseStats res = data;
    //     return res;
    // }
};


}   // namespace