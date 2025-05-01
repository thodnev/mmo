#include "png_wrap.hpp"
#include <iostream>
#include <format>

int main(int argc, char * const argv[])
{
    auto &out = std::cerr;
    out << "Test PNG wrapper\n";

    using namespace png_wrap;
    PngImage img("minimap/test4.png");
    img.load();
    out << std::format("Loaded img {} ({}x{})\n",
        (std::string)img.file, img.width, img.height);

    PngImage exp("/tmp/test4_export.png");
    exp.data = img.data;
    exp.height = img.height;
    exp.width = img.width;
    exp.save();

    out << "DONE\n";

    return 0;
}