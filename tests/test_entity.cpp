#include <iostream>
#include <format>

import entity;

void test_basic(std::ostream &out)
{
    // create
    entity::BaseStats base = {10, 20, 30, 0, 0, 0, 0, 69};
    // output
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

    out << "unpacked == base >> " << (unpacked == base) << "\n";
    out << "unpacked != base >> " << (unpacked != base) << "\n";
}

int main(const int argc, char * const argv[])
{
    std::cerr << "Test entity\n";

    test_basic(std::cerr);

    return 0;
}