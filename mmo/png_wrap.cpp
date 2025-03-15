#include "utils.hpp"
#include <png.h>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

#include <iostream>


template <typename T>
std::string to_string(T &items, const std::string sep = ", ") {
    std::string res;

    for (auto it = items.begin(); it != items.end(); ) {
        res += std::to_string(*it);
        if (++it == items.end()) {
            break;          // Avoid adding after the last string
        }
        res += sep;
    }

    return res;
}


template <typename T>
void _arrout(const T arr[], size_t sz, std::string sep = ", ") {
    std::vector<T> vec(arr, arr + sz / sizeof(arr[0]));
    std::cout << to_string(vec, sep) << std::endl;
}

class PngError : public std::exception {
private:
    std::string message;

public:
    PngError(const std::string &msg) : message(msg) {}

    // Override the what() method
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
};


template <typename flat_uint = unsigned long,
          unsigned long MAX_DIM = 4096 * 4096>
class PngImage {
public:
    //std::filesystem::path file;
    std::vector<flat_uint> data_flat;
    decltype(MAX_DIM) width, height;

//private:
public:
    void read_file(std::filesystem::path file);
};


template <typename flat_uint, unsigned long MAX_DIM>
void PngImage<flat_uint, MAX_DIM>
     ::read_file(std::filesystem::path file)
{
    std::FILE *fp = fopen(file.c_str(), "rb");
    if (nullptr == fp) {
        throw PngError("Cannot open file " + (std::string)file);
    }

    const auto READ_FAIL_MSG = "File " + (std::string)file + " read failed";

    // read bytes 0..7 and pass to libpng to check header
    std::vector<uint8_t> header(8);
    auto nrd = std::fread(&header[0], sizeof(header[0]), header.size(), fp);
    if (header.size() != nrd) {
        std::fclose(fp);
        throw PngError(READ_FAIL_MSG);
    }

    // std::cout << "Read\n";
    // std::cout << to_string(header);
    // std::cout << std::endl;

    if (png_sig_cmp(header.data(), 0, header.size())) {
        std::fclose(fp);
        throw PngError("File " + (std::string)file + " is not PNG");
    }

    png_structp png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr, nullptr, nullptr);

    if (nullptr == png_ptr) {
        std::fclose(fp);
        throw PngError("Alloc read_struct");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        std::fclose(fp);
        throw PngError("Alloc info_struct");
    }

    auto err_on = [&](bool testval, std::string msg) {
        if (testval) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            std::fclose(fp);
            throw PngError(msg);
        }
    };

    // Set error handling using the setjmp/longjmp method (libpng default).
    // REQUIRED unless own error handlers are set in the 
    // png_create_read_struct() earlier.
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        // We get back here if any error occurs
        err_on(true, READ_FAIL_MSG);
        // png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        // std::fclose(fp);
        // throw PngError(READ_FAIL_MSG);
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

    decltype(width) imwidth = png_get_image_width(png_ptr, info_ptr);
    decltype(height) imheight = png_get_image_height(png_ptr, info_ptr);

    err_on(imwidth * imheight > MAX_DIM, std::format(
        "Image size {} x {} > {}", imwidth, imheight, MAX_DIM));

    auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    err_on(bit_depth != 1, "Only 1-bit images are supported now");  // @TODO

    // number of bytes needed to hold a row
    auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // allocate matrix of imheight x rowbytes
    std::vector<std::vector<uint8_t>> matrix(imheight, std::vector<uint8_t>(rowbytes));
    // convert std::vector to a C-style array for compatibility
    std::vector<uint8_t *> c_matrix(imheight);
    for (auto i = 0; i < imheight; i++) {
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
    this->width = imwidth;
    this->height = imheight;

    // retrieve the image data
    std::cout << std::format("Image size: {} x {}\n", imwidth, imheight);
    std::cout << std::format("Bit depth: {}\n", bit_depth);
    std::cout << std::format("Row bytes: {}\n", rowbytes);

    std::cout << "\nFirst line:\n" << to_string(matrix[0]) << std::endl;

    // Flatten image data
    auto flat = utils::flatten_bits(matrix, imwidth, imheight);

    std::cout << "FLAT SIZE: " << flat.size() << std::endl;
    std::cout << "\n=========\n" << to_string(flat) << std::endl;
}


int main(const int argc, char * const argv[])
{
    std::cout << "Test" << std::endl;

    PngImage img;
    img.read_file("minimap/test4.png");
    std::cout << "DONE" << std::endl;
    return 0;
}