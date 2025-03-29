#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

import types;
import common;

using namespace common;
using namespace types;

template <typename T = int16_t, typename U = int16_t>
auto find_path(const BinMask<T> &map, Coord<U> from, Coord<U> to)
{
    std::cout << std::format("From ({}, {}) to ({}, {})\n",
        from.x, from.y, to.x, to.y);
    std::cout << std::format("On a {}x{} grid\n",
        map.width, map.height);

    // @TODO: make it Coord methods
    auto is_on_map = [&](const Coord<U> &coord) constexpr {
            return coord.x < map.width && coord.y < map.height && coord.x >= 0 && coord.y >= 0;
    };

    auto ensure_on_map = [&](const Coord<U> &coord) constexpr {
        if (! is_on_map(coord)) {
            throw std::range_error(std::format(
                "Coord {} does not belong to map {}x{}",
                (std::string)coord, map.width, map.height));
        }
    };

    auto is_forbidden = [&](const Coord<U> &coord) constexpr {
        return false == map.get_value(coord.x, coord.y);     // already taken
    };

    // Ensure coordinates belong to map
    ensure_on_map(from);
    ensure_on_map(to);          // @TODO: maybe make optional, with flag

    // Make sure we're going from free point to free point
    // @TODO: make it optional for to
    auto [fbd_from, fbd_to] = std::tuple(is_forbidden(from), is_forbidden(to));
    if (fbd_from || fbd_to) {
        throw std::range_error(std::format(
            "{} coord {} is marked on map as forbidden",
            (fbd_from ? "Starting" : "Target"),
            (std::string)(fbd_from ? from : to)
        ));
    }

    std::unordered_set<Coord<U>> visited = {from}; 
}

int main(const int argc, char * const argv[])
{
    std::cout << "Pathfinding\n";

    // Load test map
    BinMask mask("IDEAS/testmap.png");

    std::pair from = {140, 780},
              to   = {545, 640};
    
    // std::cout << "FROM HASH: " << Coord(from).hash() << std::endl;
    // std::cout << "TO HASH: " << Coord(to).hash() << std::endl; 

    find_path(mask, Coord(from), Coord(to));
    
    return 0;
}