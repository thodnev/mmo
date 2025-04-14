#include <iostream>
#include <format>

#include <cstdint>

import entity;

void test_basic(std::ostream &out)
{
    // create
    entity::BaseStats base = {10, 20, 30, 0, 0, 0, 0, 69};
    // size
    out << "BaseStats size: " << sizeof(base) << "\n";
    // output
    out << base << "\n";

    out << "BaseStats output as array:\n";
    for (const auto &el : base.as_array) {
        out << +el << " ";
    }
    out << "\n";

    out << "Setting element as array\n";
    auto &arr = base.as_array;
    arr[0] = 69; arr[1] = 96; arr[2] = 0; arr[3] = 0;
    out << base << "\n";

    out << "Setting element by .dot\n";
    base.STR = 99;
    out << base << "\n";

    // pack
    auto packed = base.to_packed();
    out << "Packed: ";
    for (const auto &el : packed)  out << +el << " ";
    out << "\n";

    // create from packed
    std::tie(packed[0], packed[1]) = std::make_tuple(6, 9);
    const auto unpacked = entity::BaseStats::from_packed(packed);
    out << "Unpacked " << unpacked << "\n";

    //out << std::boolalpha;
    out << "unpacked == base -> " << (unpacked == base) << "\n";
    out << "unpacked != base -> " << (unpacked != base) << "\n";

    // now StatsDiff
    out << "\n";
    entity::StatsDiff one = {165, 96, 69, 0, 10, 20, 30, 40};
    entity::StatsDiff two = {96, 27, 69, 0, 1, 2, 3, 4};
    out << "StatsDiff size: " << sizeof(one) << "\n";
    out << "ONE:     " << one << "\n";
    out << "TWO:     " << two << "\n";

    auto sum = one + two;
    out << "ONE+TWO: " << sum << "\n";
    out << "ONE-TWO: " << (one - two) << "\n";

    out << "Original objects after math:\n";
    out << "ONE: " << one << "\n" << "TWO: " << two << "\n";

    out << "Accessing separate values\n";
    out << "STR: " << sum.STR << ", INT: " << sum.INT << "\n";
}


int main(const int argc, char * const argv[])
{
    std::cerr << "Test entity\n";

    test_basic(std::cerr);

    return 0;
}