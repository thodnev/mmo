#include "utils.hpp"
#include <cstdint>

namespace utils {

std::vector<uint8_t> flatten_bits(
    const std::vector<std::vector<uint8_t>> &matrix,
    unsigned long width,
    unsigned long height)
{
    std::vector<uint8_t> flat;

    // first row copied as-is till last byte
    flat.insert(flat.end(), matrix[0].begin(), matrix[0].begin() + (width / 8));
    // others need to be rotated by difference
    auto nback = width % 8;
    auto carry = matrix[0].back();
    for (decltype(height) nrow = 1; nrow < height; nrow++) {
        for (size_t nb = 0; nb < matrix[0].size() - 1; nb++) {
            auto cur = matrix[nrow][nb];

            auto el = carry | (cur >> nback);
            carry = cur << (8 - nback);

            flat.push_back(el);
        }
        // last byte
        auto last = matrix[nrow].back();
        auto el = carry | (last >> nback);

        nback = (width - 8 + nback) % 8;

        carry = (0 == nback) ? 0 : el;
        if (0 == nback) {
            flat.push_back(el);
        }
    }
    // add the last byte
    flat.push_back(carry);

    // if (0 == (imwidth % 8)) {       // simple case
    //     for (const auto &row : matrix) {
    //         flat.insert(flat.end(), row.begin(), row.end());
    //     }
    // } else {
    //     // ...
    // }

    return flat;
}

}   // namespace