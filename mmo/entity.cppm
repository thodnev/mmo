module;
#include <array>
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <string>
#include <span>
#include <sstream>
#include <vector>
#include <utility>

import err;

export module entity;

// @TODO: these should probably go to other module

// @TODO: find a better name for it
/// Represents data serialization result
using bytevec = std::vector<uint8_t>;
using bytepack = std::span<const uint8_t>;

/// Constraint for a class to be considered Packabe (serializable)
template <typename T>
concept Packable = requires(T a, const bytepack &data) {
    /// Must have instance method to_packed() returning bytevec
    { a.to_packed() } -> std::same_as<bytevec>;

    /// Must have class method from_packed(bytepack &) returning an object instance 
    { T::from_packed(data) } -> std::same_as<T>;
};

export namespace entity {
// Entity (Character, Monster, NPC) related data structures


/// A base class for BaseStats and StatsDiff implementations below
template <typename T>
struct StatsCommon {
    static constexpr size_t size = 8;   ///< Number of elements to simplify access
    static constexpr std::array<std::string, size> field_names = {
        "STR", "INT", "DEX", "CON", "WIS", "AGI", "LUK", "INS"};
    using type = T;  ///< Make available in subclasses as simple alias
    using arr_t = std::array<type, size>;     ///< Alias to underlying array view

    union {
        struct {
            T STR = 0;       ///< Strength
            T INT = 0;       ///< Intelligence
            T DEX = 0;       ///< Dexterity

            T CON = 0;       ///< Constitution
            T WIS = 0;       ///< Wisdom
            T AGI = 0;       ///< Agility

            T LUK = 0;       ///< Luck
            T INS = 0;       ///< Insanity
        };
        /// Flat array representation of the fields in order
        arr_t as_array;
    };

    constexpr StatsCommon(const T str, const T int_, const T dex, const T con,
                          const T wis, const T agi, const T luk, const T ins) noexcept
        : STR(str), INT(int_), DEX(dex), CON(con), WIS(wis), AGI(agi), LUK(luk), INS(ins) {}

    constexpr StatsCommon(const arr_t &arr) noexcept : as_array(arr) {}

    constexpr StatsCommon() = default;

    constexpr bool operator==(const StatsCommon<T> &r) const noexcept
    {
        return as_array == r.as_array;
    }

    /// Flat array representation of the fields in order
    constexpr std::array<type, size> values() const noexcept
    {
        return {STR, INT, DEX, CON, WIS, AGI, LUK, INS};
    }

    /// Converts to string with a specified delimeter
    /// @param sep delimiter to place between separate fields
    constexpr const std::string to_string(const std::string &sep = ", ") const noexcept
    {
        std::string buf(128, '\0');     // Preallocate. Expected out len 86..94
        std::ostringstream out;
        out.str(buf);       // set buffer

        for (size_t idx = 0; idx < this->as_array.size(); idx++) {
            out << field_names[idx] << ": " << +this->as_array[idx];
            if (idx < this->as_array.size() - 1)  out << sep;
        }
        return out.str();
    }

    /// Allow direct casting to string, uses default delimeter from `to_string()`
    constexpr operator std::string() const noexcept
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


/// Stores Base stats of the entity
struct BaseStats : public StatsCommon<uint8_t> {
    using StatsCommon<type>::StatsCommon;    // inherit constructor

    constexpr bytevec to_packed() const noexcept
    {
        return bytevec(this->as_array.begin(), this->as_array.end());
    }

    static constexpr BaseStats from_packed(const bytepack &data)
    {
        // const auto tup = utils::vector_to_tuple<size>(data);
        // return std::apply([](auto &&...args) { return BaseStats(args...); },
        //                tup);

        if (size != data.size()) [[unlikely]] {
            throw err::ValueError("Unpacking size mismatch: expected {}, got {}",
                                  size, data.size());
        }

        std::array<type, size> arr;
        std::copy(data.begin(), data.end(), arr.begin());
        return BaseStats(arr);
    }
};
static_assert(sizeof(BaseStats) == BaseStats::size * sizeof(BaseStats::type),
              "BaseStats struct entries are not stored efficiently");
static_assert(Packable<BaseStats>,
              "BaseStats does not adhere to the Packabe interface");


struct StatsDiff : public StatsCommon<int16_t> {
    using StatsCommon<type>::StatsCommon;    // inherit constructor

    constexpr StatsDiff operator+(const StatsDiff &r) const noexcept
    {
        return from_operator(r, std::plus<type>{});
    }

    // @TODO:
    //constexpr StatsDiff& operator+=(const StatsDiff &r) noexcept

    constexpr StatsDiff operator-(const StatsDiff &r) const noexcept
    {
        return from_operator(r, std::minus<type>{});
    }

    // @TODO:
    //constexpr StatsDiff& operator-=(const StatsDiff &r) noexcept

private:
    template <typename Func>
    constexpr StatsDiff from_operator(const StatsDiff &other, Func func) const noexcept
    {
        const auto &oth = other.as_array;
        auto res = this->as_array;

        // apply func(res[i], oth[i]) and store result into res[i]
        // i.e. for func(a[i], b[i]) -> result[i] the call pattern is:
        //      (a.begin(), a.end(), b.begin(), result.begin(), func)
        std::transform(res.begin(), res.end(), oth.begin(), res.begin(), func);

        return StatsDiff(res);
    }
};
}   // namespace