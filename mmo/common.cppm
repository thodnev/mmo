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

    explicit constexpr MaskLike() noexcept
        : width(0), height(0) {}

    explicit constexpr MaskLike(const dim_t width, const dim_t height) noexcept
        : width(width), height(height) {}

    virtual ~MaskLike() = default;    ///< Important for proper cleanup in derived classes

    /// *abstract* Returns pixel at coordinates without checking for bounds
    /// (UNSAFE) This may result in out-of-bonds access, so know what you're doing.
    /// Use `get_value()` instead if bounds check needed
    virtual bool get_value_raw(const dim_t x, const dim_t y) const noexcept =0;

    /// *abstract* Sets pixel value at provided coordinates without bounds check
    /// (UNSAFE) This may result in out-of-bonds access, so know what you're doing.
    /// Use `set_value()` instead if bounds check needed
    virtual void set_value_raw(const dim_t x, const dim_t y, const bool val) noexcept =0;

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

    /// Allow indexing mask as mask[x][y] and setting value as mask[x][y] = val
    /// (UNSAFE) For improved performance and consistency with arrays,
    /// bounds are not checked. Use `get_value(x, y)` for safer, but slower access.
    constexpr decltype(auto) operator[](this auto &self, const dim_t x) noexcept
    {
        return RowIndexer{self, x};
    }

    /// Count bits set to 1.
    /// Inefficient implementation, meant to be overridden in child classes
    virtual inline size_t count_set_bits() const noexcept
    {
        size_t res = 0;
        for (dim_t x = 0; x < width; x++) {
            for (dim_t y = 0; y < height; y++) {
                res += get_value_raw(x, y);
            }
        }

        return res;
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

private:
    template <typename MaskT>
    class RowIndexer {
        private:
            MaskT &mask;
            const dim_t x;

            class MaskBit {
            private:
                MaskT &mask;
                const dim_t x, y;
            public:
                constexpr MaskBit(MaskT &mask, const dim_t x, const dim_t y) noexcept
                    : mask(mask), x(x), y(y) {}
                
                /// Set value at coordinates (x, y)
                [[gnu::always_inline]]
                constexpr bool operator=(const bool val) noexcept
                {
                    /// (!) Use unsafe raw indexing for better performance
                    mask.set_value_raw(x, y, val);
                    return val;
                }

                // Overload bool() to allow reading the value
                [[gnu::always_inline]]
                constexpr operator const bool() const noexcept
                {
                    /// (!) Use unsafe raw indexing for better performance
                    return mask.get_value_raw(x, y);
                }
            };

        public:
            constexpr RowIndexer(MaskT &mask, const dim_t x) noexcept
                : mask(mask), x(x) {}
        
            [[gnu::always_inline]]
            constexpr auto operator[](const dim_t y) const noexcept
            {
                
                return MaskBit(mask, x, y);
            }
    };
};


class BinMask : public MaskLike {
public:
    std::vector<uint8_t> flat;

    BinMask() : MaskLike(), flat() {};

    BinMask(const std::filesystem::path &file) { this->from_png(file); }

    virtual inline size_t count_set_bits() const noexcept override
    {
        return utils::count_bits(this->flat);
    }

    [[gnu::hot, gnu::always_inline]]
    virtual inline bool get_value_raw(const dim_t x, const dim_t y)
        const noexcept override
    {
        auto idx = this->index_for(x, y);
        auto byte = this->flat[idx / 8];
        return byte & (0x80 >> (idx % 8));
    }

    [[gnu::hot, gnu::always_inline]]
    virtual inline void set_value_raw(const dim_t x, const dim_t y, const bool val)
        noexcept override
    {
        auto idx = this->index_for(x, y);
        int bit = 7 - (idx % 8);
        auto &byte = this->flat[idx / 8];
        byte = (byte & ~(1 << bit)) | (val << bit);
    }

protected:
    [[gnu::always_inline]]
    constexpr inline uint32_t index_for(const dim_t x, const dim_t y) const noexcept
    {   
        static_assert(sizeof(x) + sizeof(y) <= sizeof(uint32_t), "Won't fit");
        return static_cast<uint32_t>(y) * this->width + x;
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


class IndexedBinMask : public BinMask {
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
    IndexedBinMask() : BinMask() { this->_set_indices(); }
    IndexedBinMask(const std::filesystem::path &file) : BinMask(file)
    {
        this->_set_indices();
    }

    virtual inline size_t count_set_bits() const noexcept override
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
        auto totalnum = this->count_set_bits();

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
        dim_t y = index / this->width;
        dim_t x = index - y * this->width;

        // @FIXME
        return {static_cast<Coord::type>(x), static_cast<Coord::type>(y)};
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