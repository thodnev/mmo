#include "macro.hpp"
#include <iostream>
#include <utility>
import pthfind;
import common;
import types;

using namespace pthfind;

void dump_path(const Coord startpoint, const std::vector<types::PathEntry> &path, std::ostream &out = std::cout) {
    auto cur_x = startpoint.x;
    auto cur_y = startpoint.y;
    auto i = 0;
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        const auto el = *it;
        auto [dx, dy] = el.to_coords();
        cur_x += dx;
        cur_y += dy;

        //std::cout << std::format("{:<3} [{}, {}]\t", i, cur_x, cur_y);
        //std::cout << el << std::endl;
        out << cur_x << "\t" << cur_y << std::endl;
        i++;
    }
}

void dump_visited(const std::vector<Coord> &visited, std::ostream &out = std::cout) {
    for (const auto &el : visited) {
        out << el.x << "\t" << el.y << std::endl;
    }
}

int main(const int argc, char * const argv[])
{
    std::cerr << "Test pthfind\n";

    // Load test map
    common::BinMask testmap("tests/maps/pthtest_4x.png");

    // std::pair from = {148, 257},
    //             to = {239,  19};
    // std::pair from = {108, 439},
    //             to = {186,  346};
    std::pair from = {298, 512},
                to = {466,  37};
    LOG("Looking for path ({}, {}) -> ({}, {})",
        from.first, from.second, to.first, to.second);
    // TODO: check when radius is 0
    std::vector<Coord> visited;
    auto path = pathfind_astar(testmap, Coord{(axis_t)from.first, (axis_t)from.second},
               Coord{(axis_t)to.first, (axis_t)to.second},
            560, visited);


    // dump_path(Coord{(axis_t)from.first, (axis_t)from.second},
    //           path);
    // std::cout << "\n\n";
    // dump_visited(visited);
    std::cerr << "Visited sz=" << visited.size() << ", Cap=" << visited.capacity() << "\n";
    return 0;
}