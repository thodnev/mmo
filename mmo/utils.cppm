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

import err;
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


/// Converts row-major packed bits into completely flat representation.
/// @see unflatten_bits for corresponding inverse function
/// @param matrix - Input in a form of matrix with each byte filled with data bits,
///     up to the last byte (which is filled partially if width % 8 != 0)
///         row0:  | byte 0.0 | byte 0.1 | ... | byte 0.N |
/// @param width - Image (matrix) width
/// @param height - Image (matrix) height
/// @returns Vector as a flat sequence of bytes, as if rows conatenated together,
///     with no gaps and bits filled in every byte (except the last byte, which
///     may get only partially filled, from highest to lowest bits)
///     | row0 bits | row1 bits | ... | rowN bits |
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

    return flat;
}


/// Converts a flat bits representation into row-major packed bits
/// @see flatten_bits for an inverse companion
/// @param flat - A flat vector, with each byte filled with data bits
///     as rows concatenated together and no gaps
/// @param width - Image (matrix) width
/// @param height - Image (matrix) height
/// @returns Matrix in row-major form, with bytes in rows filled with corresponding
///     bits, except for the last byte of each row (which may get filled only
///     partially, with high to low bits order).
std::vector<std::vector<uint8_t>> unflatten_bits(
    const std::vector<uint8_t> &flat,
    unsigned long width,
    unsigned long height)
{
    const auto rowbytes = (width + 7) / 8;
    const auto fullbytes = width / 8;
    const uint8_t rest_bits = width % 8;

    // preallocate
    std::vector<std::vector<uint8_t>> res(height, std::vector<uint8_t>(rowbytes));

    uint8_t carry = 0;
    uint8_t carry_bits = 0;     // number of bits currently in carry
    auto ix = flat.begin();
    for (decltype(height) nrow = 0; nrow < height; nrow++) {
        // process full bytes first
        for (decltype(width) ncol = 0; ncol < fullbytes; ncol++) {
            uint8_t byte = *ix++;

            uint8_t el = carry | (byte >> carry_bits);
            carry = (byte << (8 - carry_bits)) & 0xFF;

            res[nrow][ncol] = el;
        }

        // if dealing with complete bytes, skip the last partial byte processing
        if (rest_bits == 0)
            continue;

        // process partial (last) byte when needed
        if (carry_bits >= rest_bits) {  // carry has enough bits to fill element
            // -> no read needed
            uint8_t el = carry & ~(0xFF >> rest_bits);
            carry <<= rest_bits;
            carry_bits -= rest_bits;

            res[nrow][fullbytes] = el;
        } else {                        // carry doesn't have enough bits yet
            // -> consume what we have and read the rest
            uint8_t byte = *ix++;
            uint8_t el = carry | (byte >> carry_bits);
            el &= ~(0xFF >> rest_bits);
            carry = (byte << (rest_bits - carry_bits)) & 0xFF;
            carry_bits = 8 - (rest_bits - carry_bits);

            res[nrow][fullbytes] = el;
        }
    }

    return res;
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


template <size_t size, typename T>
auto vector_to_tuple(const std::vector<T> &vec)
{
    if (size != vec.size()) [[unlikely]] {
        throw err::LookupError("size mismatch: expected {}, got {}", size, vec.size());
    }

    return [&]<size_t... Is> (std::index_sequence<Is...>) constexpr {
        return std::make_tuple(vec[Is]...);
    }(std::make_index_sequence<size>{});
}

}   // namespace