#include <bit>
#include <concepts>
#include <cstdint>
#include <string>
#include <vector>

namespace utils {

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

std::vector<uint8_t> flatten_bits(
    const std::vector<std::vector<uint8_t>> &matrix,
    unsigned long width,
    unsigned long height
);


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

}   // namespace
