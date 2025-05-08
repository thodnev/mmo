#include "png_wrap.hpp"
#include <iostream>
#include <format>
#include <filesystem>

import utils;
import err;

int main(int argc, char * const argv[])
{
    auto &out = std::cerr;
    std::filesystem::path img_file = "minimap/test2.png";
    out << "Test PNG wrapper for '" << img_file.string() << "'\n";

    using namespace png_wrap;
    PngImage img(img_file);
    img.load();
    out << std::format("Loaded img {} ({}x{})\n",
        img.file.string(), img.width, img.height);

    std::filesystem::path exp_path = std::format("/tmp/{}_export{}",
        img.file.stem().string(), img.file.extension().string());
    PngImage exp(exp_path);
    exp.data = img.data;
    exp.height = img.height;
    exp.width = img.width;
    exp.save();
    out << "Png saved to " << exp.file.string() << std::endl;

    out << "Wrapped flat representation:\n";
    auto flat = utils::flatten_bits(img.data, img.width, img.height);
    for (const auto &el : flat) {
        out << std::format("0x{:02X} ", el);
    }
    out << "\n";

    out << "Original representation:\n";
    for (decltype(img.height) row = 0; row < img.height; row++) {
        for (const auto &el : img.data[row]) {
            out << std::format("0x{:02X} ", el);
        }
        out << "\n";
    }

    out << "Unwrapped from flat representation:\n";
    auto unwrapped = utils::unflatten_bits(flat, img.width, img.height);
    if (unwrapped.size() != img.data.size()) {
        throw err::RuntimeError("[!] Size mismatch. Exp: {}, Got: {}\n", 
                                unwrapped.size(), img.data.size());
    }
    for (size_t nrow = 0; nrow < unwrapped.size(); nrow++) {
        if (unwrapped[nrow].size() != img.data[nrow].size()) {
            throw err::RuntimeError("[!] Size mismatch @ row {}. Exp: {}, Got: {}\n", 
                                    nrow, unwrapped[nrow].size(), img.data[nrow].size());
        }
        for (size_t ncol = 0; ncol < unwrapped[nrow].size(); ncol++) {
            out << std::format("0x{:02X} ", unwrapped[nrow][ncol]);
            if (unwrapped[nrow][ncol] != img.data[nrow][ncol]) {
                throw err::RuntimeError("[!] El mismatch @[{}][{}]. Exp: 0x{:02X}, Got: 0x{:02X}\n", 
                                        nrow, ncol, img.data[nrow][ncol], unwrapped[nrow][ncol]);
            }
        }
        out << "\n";
    }

    std::filesystem::path unwr_path = std::format("/tmp/{}_unwrapped{}",
        img.file.stem().string(), img.file.extension().string());
    PngImage unwr(unwr_path);
    unwr.data = unwrapped;
    unwr.height = img.height;
    unwr.width = img.width;
    unwr.save();
    out << "Unwrapped image saved to " << unwr.file.string() << "\n";


    out << "DONE\n";

    return 0;
}