#include <iostream>

import entity;

int main(const int argc, char * const argv[])
{
    std::cerr << "Test entity\n";

    entity::BaseStats base = {10, 20, 30};
    std::cerr << base << "\n";
    return 0;
}