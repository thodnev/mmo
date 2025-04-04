#include "macro.hpp"
#include <iostream>
#include <utility>
import pthfind;
import common;

using namespace pthfind;

int main(const int argc, char * const argv[])
{
    std::cerr << "Test pthfind\n";

    // Load test map
    common::BinMask<axis_t> testmap("tests/maps/pthtest_4x.png");

    // std::pair from = {148, 257},
    //             to = {239,  19};
    std::pair from = {108, 439},
                to = {186,  346};
    LOG("Looking for path ({}, {}) -> ({}, {})",
        from.first, from.second, to.first, to.second);
    // TODO: check when radius is 0
    pathfind_astar(testmap, Coord{(axis_t)from.first, (axis_t)from.second},
               Coord{(axis_t)to.first, (axis_t)to.second},
            130);
    return 0;
}