module;
#include <bit>
#include <concepts>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include <chrono>
#include <format>
#include <iostream>
#include <string>

export module utils;
export namespace utils {

// @FIXME: probably not needed when it comes to refactoring
template <typename T>
std::string to_string(T &items, const std::string sep = ", ") {
    std::string res;

    for (auto it = items.begin(); it != items.end(); ) {
        res += std::to_string(*it);
        if (++it == items.end()) {
            break;          // Avoid adding after the last string
        }
        res += sep;
    }

    return res;
}


template<std::integral T>
size_t count_bits(const T data[], const size_t len)
{
    size_t res = 0;
    for (size_t n = 0; n < len; n++) {
        res += std::popcount(data[n]);
    }
    return res;
}

template<std::integral T>
size_t count_bits(const std::vector<T> &vec)
{
    return count_bits(vec.data(), vec.size());
    // size_t res = 0;
    // for (auto el : &vec) {
    //     res += std::popcount(el);
    // }
    // return res;
}


std::vector<uint8_t> flatten_bits(
    const std::vector<std::vector<uint8_t>> &matrix,
    unsigned long width,
    unsigned long height)
{
    std::vector<uint8_t> flat;
    flat.reserve((width * height + 7) / 8); // Preallocate memory for efficiency

    size_t n_full_bytes = width / 8;
    size_t n_extra_bits = width % 8;
    // std::cerr << std::format("{} full bytes, {} extra bits\n", n_full_bytes, n_extra_bits);

    // first row copied as-is till last byte
    // flat.insert(flat.end(), matrix[0].begin(), matrix[0].begin() + (width / 8));

    uint8_t carry = 0;
    uint8_t carry_bits = 0;     // number of bits in carry
    for (size_t nrow = 0; nrow < height; nrow++) {
        // first process full bytes part
        for (size_t nb = 0; nb < n_full_bytes; nb++) {
            uint8_t byte = matrix[nrow][nb];

            // merge with carried from previous row
            uint8_t packed = carry | (byte >> carry_bits);
            flat.push_back(packed);

            // update carry
            carry = byte << (8 - carry_bits);
        }

        if (n_extra_bits) {
            uint8_t last_byte = matrix[nrow][n_full_bytes]; // & (0xFF << (8 - n_extra_bits));
            
            uint8_t last_packed = carry | (last_byte >> carry_bits);
            if ((n_extra_bits + carry_bits) >= 8) {     // we collected enough to commit
                flat.push_back(last_packed);
                // carry will store the risidual part of current byte
                carry = last_byte << (8 - carry_bits);
                carry_bits = n_extra_bits + carry_bits - 8;
            } else {
                // append current bits to carry and increase carry_bits
                carry = last_packed;
                carry_bits += n_extra_bits;
            }
        }
    }
    // Push the last carry byte if needed
    if (carry_bits > 0) {
        flat.push_back(carry);
    }

    // if (0 == (imwidth % 8)) {       // simple case
    //     for (const auto &row : matrix) {
    //         flat.insert(flat.end(), row.begin(), row.end());
    //     }
    // } else {
    //     // ...
    // }

    return flat;
}


class TimeIt {
public:
    using point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using delta_t = std::chrono::duration<double>;

    point_t last;
    delta_t total;
    bool is_running;

    explicit TimeIt(const bool is_autostart = true) : total(0), is_running(false)
    {
        if (is_autostart)  measure_from();
    }

    /// Starts timer for new measure
    constexpr inline void measure_from()
    {
        last = std::chrono::high_resolution_clock::now();
        is_running = true;
    }

    /// Adds passed interval to the total time
    constexpr inline void measure_append()
    {
        auto now = std::chrono::high_resolution_clock::now();
        total += now - last;
        last = now;
        if (! is_running) {
            throw std::runtime_error("Timer is not running");
        }
    }

    constexpr inline void stop()
    {
        is_running = false;
    }

    /// Returns total time of all measurements
    constexpr auto took()
    {
        if (is_running) measure_append();
        stop();
        auto seconds = total.count();
        return seconds;
    }

    /// Shows information on total time
    constexpr void report_took(const std::string &name = "", std::ostream &out = std::cerr)
    {
        auto tm = took();
        const auto ops = 1.0 / tm;

        std::string res;
        std::vector<std::string> prefixes = {"s", "ms", "us", "ns"};
        for (const auto &prefix : prefixes) {
            if (prefix == "ns") {
                res = std::format("{:.0f} ", tm) + prefix;
                break;
            } else if ((unsigned long)tm > 0) {
                res = std::format("{:.3f} ", tm) + prefix;
                break;
            } else {
                tm *= 1000;
            } 
        }
        res += std::format(" ({:.2e} ops/s)", ops);
        
        auto msg = (name == "") ? "" : (name + " ");
        out << "TIMEIT: " << msg << "took " << res << std::endl;
    }
};

}   // namespace