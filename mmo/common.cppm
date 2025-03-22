module;
#include "macro.hpp"
#include "png_wrap.hpp"
#include "utils.hpp"
#include <cstdint>
#include <filesystem>
#include <limits>
#include <variant>
#include <vector>

//#include <iostream>
export module common;

import types;

export namespace common {
    using types::Coord;   // Make available in this namespace


template <typename T = unsigned long>
class BinMask {
public:
    std::vector<uint8_t> flat;
    T width, height;

    BinMask() : flat(), width(0), height(0) {};

    BinMask(const std::filesystem::path &file) { this->from_png(file); }

    size_t num_set_bits();

    bool get_value(const T x, const T y);

private:
    void from_png(const std::filesystem::path &file);

};


template <typename T = unsigned long>
class IndexedBinMask : public BinMask<T> {
public:
    std::variant<
        //std::monostate,     // prevent default initialization of vectors
        std::vector<uint8_t>,
        std::vector<uint16_t>,
        std::vector<uint32_t>,
        std::vector<uint64_t>
        >   indices_set_bits;

    // @TODO: combine constructors into one
    //        using const std::filesystem::path &file = {}
    IndexedBinMask() : BinMask<T>() { this->_set_indices(); }
    IndexedBinMask(const std::filesystem::path &file) : BinMask<T>(file)
    {
        this->_set_indices();
    }

    size_t num_set_bits();

    // returns coordinates of i-th non-zero element
    Coord<T> get_coord_nonzero(const size_t elnum);

private:
    void _set_indices();
};


template <typename T>
void BinMask<T>::from_png(const std::filesystem::path &file)
{
    png_wrap::PngImage img(file);
    this->flat = utils::flatten_bits(img.data, img.width, img.height);
    this->width = img.width;
    this->height = img.height;
}


template <typename T>
size_t BinMask<T>::num_set_bits()
{
    return utils::count_bits(this->flat);
}


template <typename T>
bool BinMask<T>::get_value(const T x, const T y)
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

template <typename T>
void IndexedBinMask<T>::_set_indices()
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

    for (size_t nbit = 0; nbit < dim; nbit++) {
        auto byte = this->flat[nbit / 8];
        auto idx = 7 - (nbit % 8);
        if (byte & (1 << idx)) {
            // Access the correct vector type using std::visit
            std::visit([nbit](auto &vec) {
                vec.push_back(nbit);
            }, this->indices_set_bits);
        }
    }
}


template <typename T>
size_t IndexedBinMask<T>::num_set_bits()
{
    size_t totalnum;
    std::visit([&totalnum](auto &vec) {
        totalnum = vec.size();
    }, this->indices_set_bits);

    return totalnum;
}


template <typename T>
Coord<T> IndexedBinMask<T>::get_coord_nonzero(const size_t elnum)
{
    auto totalnum = this->num_set_bits();

    if (elnum >= totalnum) {
        throw std::out_of_range(std::format(
            "Element {} >= {}", elnum, totalnum
        ));
    }

    // get index in flat array
    size_t index;
    std::visit([&index, elnum](auto &vec) {
        index = vec[elnum];
    }, this->indices_set_bits);

    // transform flat index to coordinate pair
    auto y = index / this->width;
    auto x = index - y * this->width;

    return {x, y};
}

};      // namespace