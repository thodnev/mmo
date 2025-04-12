module;
#include <array>
#include <concepts>
#include <cstdint>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <utility>

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

    constexpr StatsCommon(const T str, const T int_, const T dex, const T con,
                          const T wis, const T agi, const T luk, const T ins)
        : STR(str), INT(int_), DEX(dex), CON(con), WIS(wis), AGI(agi), LUK(luk), INS(ins) {}

    constexpr StatsCommon() = default;

    constexpr bool operator==(const StatsCommon<T> &r) const noexcept
    {
        return ((STR == r.STR) && (INT == r.INT) && (DEX == r.DEX) && (CON == r.CON)
             && (WIS == r.WIS) && (AGI == r.AGI) && (LUK == r.LUK) && (INS == r.INS));
    }

    /// Flat array representation of the fields in order
    constexpr const std::array<type, size> values() const noexcept
    {
        return {STR, INT, DEX, CON, WIS, AGI, LUK, INS};
    }

    /// Converts to string with a specified delimeter
    /// @param sep delimiter to place between separate fields
    constexpr const std::string to_string(const std::string &sep = ", ") const noexcept
    {
        const auto vals = values();
        std::ostringstream out;
        for (size_t idx = 0; idx < vals.size(); idx++) {
            out << field_names[idx] << ": " << static_cast<unsigned int>(vals[idx]);
            if (idx < vals.size() - 1)  out << sep;
        }
        return out.str();
    }

    /// Allow direct casting to string, uses default delimeter from `to_string()`
    operator std::string() const noexcept
    {
        return to_string();
    }

    /// Allow output to ostream
    friend std::ostream& operator<<(std::ostream &out, const StatsCommon &obj)
    {
        out << static_cast<std::string>(obj);
        return out;
    }
};


struct BaseStats : public StatsCommon<uint8_t> {
    using StatsCommon<type>::StatsCommon;    // inherit constructor

    constexpr packed_t to_packed() const noexcept
    {
        const auto vals = this->values();
        packed_t vec(vals.begin(), vals.end());
        return vec;
    }

    constexpr static BaseStats from_packed(const packed_t &data)
    {
        // const auto tup = utils::vector_to_tuple<size>(data);
        // return std::apply([](auto &&...args) { return BaseStats(args...); },
        //                tup);

        if (size != data.size()) [[unlikely]] {
            throw err::ValueError("Unpacking size mismatch: expected {}, got {}",
                size, data.size());
        }

        const auto built = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return BaseStats(data[Is]...);
        }(std::make_index_sequence<size>{});

        return built;
    }
};


}   // namespace