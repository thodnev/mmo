#include "png_wrap.hpp"
#include <png.h>

import err;

namespace png_wrap {
PngImageData load_png(const std::filesystem::path &file,
                             const size_t MAX_DIM = 4096 * 4096)
{
    std::FILE *fp = fopen(file.c_str(), "rb");
    if (nullptr == fp) {
        throw err::PngError("Cannot open file {}", (std::string)file);
    }

    // read bytes 0..7 and pass to libpng to check header
    std::vector<uint8_t> header(8);
    auto nrd = std::fread(&header[0], sizeof(header[0]), header.size(), fp);
    if (header.size() != nrd) {
        std::fclose(fp);
        throw err::PngError("File {} read failed", (std::string)file);
    }

    if (png_sig_cmp(header.data(), 0, header.size())) {
        std::fclose(fp);
        throw err::PngError("File {} is not PNG",  (std::string)file);
    }

    png_structp png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr, nullptr, nullptr);

    if (nullptr == png_ptr) {
        std::fclose(fp);
        throw err::PngError("Alloc read_struct");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        std::fclose(fp);
        throw err::PngError("Alloc info_struct");
    }

    auto err_on = [&]<typename ...Args>(bool testval, std::format_string<Args...> msg, Args && ...args) {
        if (!testval)
            return;
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::fclose(fp);
        throw err::PngError(msg, std::forward<Args>(args)...);
    };

    // Set error handling using the setjmp/longjmp method (libpng default).
    // REQUIRED unless own error handlers are set in the 
    // png_create_read_struct() earlier.
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        // We get back here if any error occurs
        err_on(true, "libpng error reading {}", (std::string)file);
        // png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        // std::fclose(fp);
        // throw err::PngError(READ_FAIL_MSG);
    }

    // Set up the input control for using standard C streams
    png_init_io(png_ptr, fp);

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

    err_on(res.width * res.height > MAX_DIM, "Image size {} x {} > {}",
           res.width, res.height, MAX_DIM);

    auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    err_on(bit_depth != 1, "Only 1-bit images are supported now");  // @TODO

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
    std::fclose(fp);

    // set own data
    res.data = matrix;

    return res;
}

void PngImage::load()
{
    auto res = load_png(file);
    this->data = res.data;
    this->width = res.width;
    this->height = res.height;
}
    
}   // namespace