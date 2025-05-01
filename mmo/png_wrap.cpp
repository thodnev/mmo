#include "png_wrap.hpp"
#include <png.h>
#include <zlib.h>

#include <format>
#include <fstream>
#include "macro.hpp"

import err;

// @TODO: 
// - adapt to reading from any input file stream
// - custom error functions and default C++ mem allocator instead of libpng defaults
// - save to file implementation

namespace png_wrap {

/// A helper to get the stream object reference out of png_struct
template <typename stream_t>
static constexpr stream_t& _png_get_stream(png_structp png_ptr)
{
    // Returns (void *) but we know what it is, since we passed it ourselves
    void *stream_ptr = png_get_io_ptr(png_ptr);
    stream_t &stream = *static_cast<stream_t *>(stream_ptr);
    return stream;
}

/// libpng custom user_read_data function used in png_set_read_fn
template <typename stream_t = std::ifstream>
static void _png_read_data(png_structp png_ptr, png_bytep data, size_t length)
{
    auto &inp = _png_get_stream<stream_t>(png_ptr);
    inp.read(reinterpret_cast<typename stream_t::char_type *>(data), length);
}

/// libpng custom user_write_data function used in png_set_write_fn
template <typename stream_t = std::ofstream>
static void _png_write_data(png_structp png_ptr, png_bytep data, size_t length)
{
    auto &out = _png_get_stream<stream_t>(png_ptr);
    out.write(reinterpret_cast<typename stream_t::char_type *>(data), length);
}

/// libpng custom user_flush_data function used in png_set_write_fn
template <typename stream_t = std::ofstream>
static void _png_flush_data(png_structp png_ptr)
{
    auto &out = _png_get_stream<stream_t>(png_ptr);
    out.flush();
}

static PngImageData load_png(const std::filesystem::path &file_path,
                             const size_t MAX_DIM = 4096 * 4096)
{
    // Rely on RAII to automatically close the file on scope exit
    std::ifstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        throw err::PngError("Cannot open file {}", (std::string)file_path);
    }

    // read bytes 0..7 and pass to libpng to check header
    std::vector<uint8_t> header(8);
    file.read(reinterpret_cast<char *>(&header[0]), header.size());
    // LOG("Read size: {}, header size: {}", file.gcount(), header.size());
    if (static_cast<std::streamsize>(header.size()) != file.gcount()) {
        throw err::PngError("File {} read failed", (std::string)file_path);
    }

    if (png_sig_cmp(header.data(), 0, header.size())) {
        throw err::PngError("File {} is not PNG",  (std::string)file_path);
    }

    png_structp png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr, nullptr, nullptr);

    if (nullptr == png_ptr) {
        throw err::PngError("Alloc read_struct");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        throw err::PngError("Alloc info_struct");
    }

    auto err_on = [&]<typename ...Args>(bool testval, std::format_string<Args...> msg, Args && ...args) {
        if (!testval)
            return;
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        throw err::PngError(msg, std::forward<Args>(args)...);
    };

    // Set error handling using the setjmp/longjmp method (libpng default).
    // REQUIRED unless own error handlers are set in the 
    // png_create_read_struct() earlier.
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        // We get back here if any error occurs
        err_on(true, "libpng error reading {}", (std::string)file_path);
    }

    // Set up the input control for using standard C streams
    //png_init_io(png_ptr, fp);

    png_set_read_fn(png_ptr, &file, _png_read_data<decltype(file)>);

    // We have already read some of the signature
    png_set_sig_bytes(png_ptr, header.size());

    // We have enough memory and we need only [PNG_TRANSFORM_* bits] transforms
    // (excludes quantizing, filling, setting background, and doing gamma
    // adjustment).
    // So we read the entire image (including pixels) into the info structure.
    // png_read_png(png_ptr, info_ptr, 0, nullptr);
    // OR:
    png_read_info(png_ptr, info_ptr);

    // Invert monochrome files to have 0 as white and 1 as black.
    // png_set_invert_mono(png_ptr);

    PngImageData res;

    res.width = png_get_image_width(png_ptr, info_ptr);
    res.height = png_get_image_height(png_ptr, info_ptr);

    err_on(res.width * res.height > MAX_DIM, "Image size {}x{} > {}",
           res.width, res.height, MAX_DIM);

    res.bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    err_on(res.bit_depth != 1, "Only 1-bit images are supported now");  // @TODO

    // number of bytes needed to hold a row
    auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // allocate matrix of imheight x rowbytes
    std::vector<std::vector<uint8_t>> matrix(res.height, std::vector<uint8_t>(rowbytes));
    // convert std::vector to a C-style array for compatibility
    std::vector<uint8_t *> c_matrix(res.height);
    for (decltype(res.height) i = 0; i < res.height; i++) {
        c_matrix[i] = matrix[i].data(); // pointer to the first element of each row
    }
    
    // One of read methods is REQUIRED. Read the entire image at once 
    png_read_image(png_ptr, c_matrix.data());

    // Read rest of file, and get additional chunks in info_ptr.  REQUIRED.
    png_read_end(png_ptr, info_ptr);

    // now the entire image is read

    // clean up after the read, and free any memory allocated.  REQUIRED.
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    // close the file
    // std::fclose(fp);

    // set own data
    res.data = matrix;

    return res;
}


static void save_png(const PngImageData &img, const std::filesystem::path &file_path)
{
    // Some sanity checks
    if (img.width == 0 || img.height == 0 || img.data.size() != img.height)
        throw err::ValueError("Image has wrong dimensions {}x{}", img.width, img.height);
    if (img.bit_depth != 1)
        throw err::ValueError("Only 1-bit images are supported by now");

    // Rely on RAII to automatically close file whenever we exit the scope
    // open overwriting the file if it already exists
    std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
    
    const std::string filename = file_path;

    if (!file.is_open()) {
        throw err::FSError("Unable to open file {}", filename);
    }

    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    auto err_on = [&](bool testval, const auto &exc) {
        if (!testval)
            return;
        // libpng can handle NULLs ok
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw exc;
    };

    // Allocate write struct
    png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr, nullptr, nullptr);
    err_on(nullptr == png_ptr, err::RuntimeError
           ("Alloc png_ptr (libpng file {})", filename));

    // Now info struct
    info_ptr = png_create_info_struct(png_ptr);
    err_on(nullptr == info_ptr, err::RuntimeError
           ("Alloc info_ptr (libpng file {})", filename));

    // Set error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        // We will get here if some error occurs
        err_on(true, err::PngError("libpng error writing file {}", filename));
    }

    // Set I/O to our custom wrappers, file will be passed around as (void *)
    png_set_write_fn(png_ptr, &file,
                     _png_write_data<std::ofstream>,
                     _png_flush_data<std::ofstream>);

    // Trade some speed for achieving the minimum file size
    png_set_compression_level(png_ptr,Z_BEST_COMPRESSION);
    
    // Set information header
    png_set_IHDR(png_ptr, info_ptr, 
        img.width, img.height, img.bit_depth, PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    // Write the file header information. REQUIRED
    png_write_info(png_ptr, info_ptr);
    // Check for write errors
    err_on(!file.good(), err::FSError
           ("Error writing png header for file {}", filename));

    // Invert monochrome pixels
    // png_set_invert_mono(png_ptr);

    // Create a C-style row pointers array for data
    std::vector<uint8_t *> c_matrix(img.data.size());
    for (decltype(img.height) i = 0; i < img.height; i++) {
        // pointer to the first element of each row
        c_matrix[i] = const_cast<uint8_t *>(img.data[i].data());
    }

    // Write image at once
    png_write_image(png_ptr, c_matrix.data());

    // Finish writing. REQUIRED
    png_write_end(png_ptr, info_ptr);

    // Check for write errors once again
    err_on(!file.good(), err::FSError
           ("Error writing png data for file {}", filename));
    
    // Deallocate resources. REQUIRED
    png_destroy_write_struct(&png_ptr, &info_ptr);
}


void PngImage::load()
{
    auto res = load_png(file);
    this->data = res.data;
    this->width = res.width;
    this->height = res.height;
}

void PngImage::save()
{
    PngImageData img = {
        .width = this->width,
        .height = this->height,
        .bit_depth = 1,     // @TODO: other depths not supported for now
        .data = this->data
    };

    save_png(img, this->file);
}
    
}   // namespace