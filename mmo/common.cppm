module;
#include "macro.hpp"
#include "png_wrap.hpp"
#include <cstdint>
#include <filesystem>
#include <limits>
#include <variant>
#include <vector>

import err;
import types;
import utils;
export module common;

export namespace common {

using types::Coord;   // Make available in this namespace


/// Interface defining common core functionality required from all BinMask classes
class MaskLike {
public:
    // 16 bits should be enough for holding up to 4 GiB binary images
    using dim_t = uint16_t;    ///< Dimension type alias

    dim_t width, height;       ///< Actual dimensions

    virtual ~MaskLike() =0;    ///< Important for proper cleanup in derived classes

    /// *abstract* Returns pixel at coordinates without checking for bounds
    virtual constexpr bool get_value_raw(const dim_t x, const dim_t y) const noexcept =0;

    /// *abstract* Sets pixel value at provided coordinates without bounds check
    virtual constexpr void set_value_raw(const dim_t x, const dim_t y, const bool val)
        noexcept =0;

    /// Returns pixel value for given coordinates, safely checking for bounds
    constexpr bool get_value(const dim_t x, const dim_t y) const
    {
        this->ensure_in_bounds(x, y);
        [[likely]] return this->get_value_raw(x, y);
    }

    /// Sets pixel at coordinates to a given value, safely checking for bounds
    constexpr void set_value(const dim_t x, const dim_t y, const bool val)
    {
        this->ensure_in_bounds(x, y);
        [[likely]] this->set_value_raw(x, y, val);
    }

    constexpr auto operator[](const dim_t x) const
    {
        return RowIndexer{*this, x};
    }

    /// Checks whether provided coordinates fit into mask dimension bounds
    [[gnu::always_inline]]
    constexpr inline bool is_in_bounds(const dim_t x, const dim_t y) const noexcept
    {
        return (x < this->width) && (y < this->height);
    }

    /// Ensures given coordinates fit into mask dimension bounds,
    /// raising error if they aren't
    [[gnu::always_inline]]
    constexpr inline void ensure_in_bounds(const dim_t x, const dim_t y) const
    {
        if (! is_in_bounds(x, y)) [[unlikely]] {
            throw err::BoundsError("Coordinates ({}, {}) out of {}x{} bounds",
                                   x, y, this->width, this->height);
        }
    }

// @TODO: add pixel set: mask[i][j] = val
private:
    class RowIndexer {
        private:
            const MaskLike &mask;
            const dim_t x;

        public:
            RowIndexer(const MaskLike &mask, const dim_t x) noexcept
                : mask(mask), x(x) {}
        
            [[gnu::always_inline]]
            constexpr bool operator[](const dim_t y) const
            {
                /// (!) Use unsafe raw indexing for better performance
                return mask.get_value_raw(x, y);
            }
    };
};

template <typename T = unsigned long>
class BinMask {
public:
    std::vector<uint8_t> flat;
    T width, height;

    BinMask() : flat(), width(0), height(0) {};

    BinMask(const std::filesystem::path &file) { this->from_png(file); }

    size_t num_set_bits()
    {
        return utils::count_bits(this->flat);
    }

    /// Gets value from coordinate pair, without checking for coordinate bounds
    /// (UNSAFE) This may result in out-of-bonds access, so know what you're doing.
    /// Use `get_value()` instead if bounds check needed
    [[gnu::hot, gnu::always_inline]]
    constexpr inline bool get_value_raw(const T x, const T y) const noexcept
    {
        size_t idx = y * this->width + x;
        auto byte = this->flat[idx / 8];
        return byte & (1 << (7 - (idx % 8)));
    }

    /// Same as `get_value_raw()`, but performs boundary checks
    constexpr inline bool get_value(const T x, const T y) const
    {
        if ((x >= this->width) || (y >= this->height)) {
            throw std::out_of_range(std::format(
                "Coordinates ({}, {}) out of {}x{} size",
                x, y, this->width, this->height
            ));
        }
    
        return this->get_value_raw(x, y);
    }

private:
    void from_png(const std::filesystem::path &file)
    {
        png_wrap::PngImage img(file);
        this->flat = utils::flatten_bits(img.data, img.width, img.height);
        this->width = img.width;
        this->height = img.height;
    }

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

    size_t num_set_bits()
    {
        size_t totalnum;
        std::visit([&totalnum](auto &vec) {
            totalnum = vec.size();
        }, this->indices_set_bits);

        return totalnum;
    }

    // returns coordinates of i-th non-zero element
    Coord get_coord_nonzero(const size_t elnum)
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

private:
    void _set_indices()
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
    
};

};      // namespace