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

    // constexpr StatsCommon(const std::array<type, size> &arr) noexcept
    // // stupid, but avoids overhead
    // : STR(arr[0]), INT(arr[1]), DEX(arr[2]), CON(arr[3]),
    //   WIS(arr[4]), AGI(arr[5]), LUK(arr[6]), INS(arr[7]) {}

    constexpr StatsCommon(const std::array<type, size> &arr) noexcept
        : StatsCommon(from_array(arr)) {}

    constexpr bool operator==(const StatsCommon<T> &r) const noexcept
    {
        return ((STR == r.STR) && (INT == r.INT) && (DEX == r.DEX) && (CON == r.CON)
             && (WIS == r.WIS) && (AGI == r.AGI) && (LUK == r.LUK) && (INS == r.INS));
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

protected:
    static constexpr StatsCommon from_array(const std::array<type, size> &arr) noexcept
    {
        const auto built = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return StatsCommon(arr[Is]...);
        }(std::make_index_sequence<size>{});

        return built;
    }
};


struct BaseStats : public StatsCommon<uint8_t> {
    using StatsCommon<type>::StatsCommon;    // inherit constructor

    constexpr bytevec to_packed() const noexcept
    {
        const auto vals = this->values();
        return bytevec(vals.begin(), vals.end());
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
        const auto oth = other.values();
        auto res = this->values();

        // apply func(res[i], oth[i]) and store result into res[i]
        // i.e. for func(a[i], b[i]) -> result[i] the call pattern is:
        //      (a.begin(), a.end(), b.begin(), result.begin(), func)
        std::transform(res.begin(), res.end(), oth.begin(), res.begin(), func);

        return StatsDiff(res);
    }
};
}   // namespace