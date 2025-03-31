#include "utils.hpp"
#include <cstdint>

namespace utils {

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

}   // namespace