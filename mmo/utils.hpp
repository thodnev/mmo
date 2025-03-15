#include <cstdint>
#include <vector>

namespace utils {

std::vector<uint8_t> flatten_bits(
    const std::vector<std::vector<uint8_t>> &matrix,
    unsigned long width,
    unsigned long height
);

}   // namespace
