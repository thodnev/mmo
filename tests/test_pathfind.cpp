#include <cmath>
#include <compare>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <queue>
#include <vector>

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

    // Coord::dist_metric return type
    using dist_t = std::invoke_result_t<
        decltype(&Coord<U>::dist_metric), Coord<U>,
        const Coord<U>&>;
    
    // This stores our evaluated paths
    struct PathDist {
        dist_t dist;
        Coord<U> last_coord;
        std::vector<PathEntry> path;
    };

    // moving steps, all possible combinations
    const auto steps = PathEntry::get_steps_table();

    class HeapCmp {
        Coord<U> endpoint;

        bool operator()( const PathDist &lhs, const PathDist &rhs ) const
        {
            auto cur = lhs.dist + lhs.last_coord.dist_metric(endpoint);
            auto oth = rhs.dist + rhs.last_coord.dist_metric(endpoint);
            return lhs > rhs;
        }
    };
    
    // heap queue with .top() always returning the PathDist with smallest distance
    std::priority_queue<PathDist, std::vector<PathDist>, HeapCmp(to)>
        heapq;
    
    heapq.emplace(PathDist{.dist = 0, .last_coord = from, .path = {}});

    // stored coords we already visited
    std::unordered_set<Coord<U>> visited;

    // traverse the map
    while (! heapq.empty()) {
        auto pd = heapq.top();

        // when node is the goal, return the found path
        if (pd.last_coord == to) return pd.path;

        // move the node to visited
        visited.emplace(pd.last_coord);
        heapq.pop();

        // for each neighbor of the current node
        for (const auto &[dx, dy]: steps) {
            Coord<U> newstep = {pd.last_coord.x + dx, pd.last_coord.y + dy};
            
            // ensure we can go here
            if (!is_on_map(newstep) || is_forbidden(newstep))  continue;
            
            // path up to the prev point + from prev point to current
            dist_t newdist = pd.dist + pd.last_coord.dist_metric(newstep);

            // TODO: check that new distance is shorter than in visited

            // if neighbor is not in heapq, add it
            if (visited.contains(newstep))  continue;

            std::vector<PathEntry> newpath = pd.path;
            newpath.push_back(PathEntry(dx, dy));
            PathDist newel = {.dist = newdist, .last_coord = newstep, .path = newpath};
            heapq.emplace(newel);
        }
    }
    std::cout << "FINISH TRAVERSAL\n";
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