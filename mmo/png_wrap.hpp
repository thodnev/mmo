#include <filesystem>
#include <vector>
#include <cstdint>

namespace png_wrap {

struct PngImageData {
    unsigned long width, height;
    uint8_t bit_depth;
    std::vector<std::vector<uint8_t>> data;
};


class PngImage {
public:
    unsigned long width, height;
    const std::filesystem::path file;
    std::vector<std::vector<uint8_t>> data;

    PngImage() : width(0), height(0), file(), data() {}

    PngImage(const std::filesystem::path &file)
        : width(0), height(0), file(file), data() {}
    
    /// Loads image from file
    void load();

    /// Saves image to file
    void save();
};

}   // namespace