module;
#include "macro.hpp"
#include "png_wrap.hpp"
#include "utils.hpp"
#include <cstdint>
#include <format>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <variant>
#include <vector>

//#include <iostream>
export module common;
export namespace common {

template <typename T = unsigned long, unsigned long DEF_UPDATE_EVERY = 64>
class RandGen {
public:
    uint64_t last = 0;

    std::function<const T()> random_func;
    decltype(DEF_UPDATE_EVERY) update_every, update_cnt;

    RandGen(decltype(random_func) random_func, decltype(update_every) update_every = DEF_UPDATE_EVERY)
        : random_func(random_func), update_every(update_every)
    {
        update();
    }

    T update()
    {
        update_cnt = 0;
        last ^= random_func();
        //std::cout << " (UPDATE) ";
        return last;
    }
    
    T random(T from = std::numeric_limits<T>::min(),
             T to = std::numeric_limits<T>::max())
    {
        //std::cout << " [" << this->last << " @ " << this->update_every << " @ " << this->update_cnt << "] ";
        return (randval() % (to - from + 1)) + from;
    }

private:
    T randval()
    {
        if (++update_cnt >= update_every) {
            update();
        }

        // MMIX by Donald Knuth, LCG generator
        last = last * 6364136223846793005 + 1442695040888963407;
        return last;
    }
};


class BinMask {
public:
    using coord_t = unsigned long;

    std::vector<uint8_t> flat;
    coord_t width, height;
    std::variant<
        std::vector<uint8_t>,
        std::vector<uint16_t>,
        std::vector<uint32_t>,
        std::vector<uint64_t>
        >   indices_set_bits;

    BinMask() : flat(), width(0), height(0) {};

    BinMask(const std::filesystem::path &file) { this->from_png(file); }
    
    size_t num_set_bits();

    bool get_value(const coord_t x, const coord_t y);
    // returns coordinates of i-th non-zero element
    std::pair<coord_t, coord_t> get_coords_nonzero(const size_t elnum);

private:
    void from_png(const std::filesystem::path &file);
    void _set_indices();
};


void BinMask::from_png(const std::filesystem::path &file)
{
    png_wrap::PngImage img(file);
    this->flat = utils::flatten_bits(img.data, img.width, img.height);
    this->width = img.width;
    this->height = img.height;
    this->_set_indices();
}

void BinMask::_set_indices()
{
    //this->num_set_bits = utils::count_bits(this->flat);

    size_t dim = this->width * this->height;
    if (dim >= std::numeric_limits<uint32_t>::max()) {
        this->indices_set_bits = std::vector<uint64_t>();
        LOG("Chosen {}-bit vector", 64);
    } else if (dim >= std::numeric_limits<uint16_t>::max()) {
        this->indices_set_bits = std::vector<uint32_t>();
        LOG("Chosen {}-bit vector", 32);
    } else if (dim >= std::numeric_limits<uint8_t>::max()) {
        this->indices_set_bits = std::vector<uint32_t>();
        LOG("Chosen {}-bit vector", 16);
    } else {
        this->indices_set_bits = std::vector<uint8_t>();
        LOG("Chosen {}-bit vector", 8);
    }

    size_t found = 0;
    for (size_t nbit = 0; nbit < dim; nbit++) {
        auto byte = this->flat[nbit / 8];
        auto idx = 7 - (nbit % 8);
        if (byte & (1 << idx)) {
            // Access the correct vector type using std::visit
            std::visit([nbit](auto& vec) {
                vec.push_back(nbit);
            }, this->indices_set_bits);

            found++;
        }
    }
}

size_t BinMask::num_set_bits()
{
    size_t totalnum;
    std::visit([&totalnum](auto& vec) {
        totalnum = vec.size();
    }, this->indices_set_bits);

    return totalnum;
}


bool BinMask::get_value(const coord_t x, const coord_t y)
{
    if ((x >= this->width) || (y >= this->height)) {
        throw std::out_of_range(std::format(
            "Coordinates ({}, {}) out of {}x{} size",
            x, y, this->width, this->height
            ));
    }

    size_t idx = y * this->width + x;
    auto byte = this->flat[idx / 8];
    return byte & (1 << (7 - (idx % 8)));
}


std::pair<BinMask::coord_t, BinMask::coord_t> BinMask
    ::get_coords_nonzero(const size_t elnum)
{
    auto totalnum = this->num_set_bits();

    if (elnum >= totalnum) {
        throw std::out_of_range(std::format(
            "Element {} >= {}", elnum, totalnum
            ));
    }

    // get index in flat array
    size_t index;
    std::visit([&index, elnum](auto& vec) {
        index = vec[elnum];
    }, this->indices_set_bits);

    // transform flat index to coordinate pair
    auto y = index / this->width;
    auto x = index - y * this->width;

    return {x, y};
}

};      // namespace