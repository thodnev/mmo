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
    std::cerr << std::format("From ({}, {}) to ({}, {})\n",
        from.x, from.y, to.x, to.y);
    std::cerr << std::format("On a {}x{} grid\n",
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
        dist_t total_dist;  ///< real_distance + evaluated_distance
        Coord<U> last_coord;
        std::vector<PathEntry> path;

        std::strong_ordering operator<=>(const PathDist &other) const
        {
            return total_dist <=> other.total_dist;
        }
    };

    // moving steps, all possible combinations
    const auto steps = PathEntry::get_steps_table();
    
    // heap queue with .top() always returning the PathDist with smallest distance
    std::priority_queue<PathDist, std::vector<PathDist>, std::greater<PathDist>>
        heapq;
    
    heapq.emplace(PathDist{.total_dist = from.dist_metric(to), .last_coord = from, .path = {}});

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

        auto pd_pure_dist = pd.total_dist - pd.last_coord.dist_metric(to);

        // for each neighbor of the current node
        for (const auto &[dx, dy]: steps) {
            Coord<U> newstep = {static_cast<U>(pd.last_coord.x + dx),
                                static_cast<U>(pd.last_coord.y + dy)};
            
            // ensure we can go here
            if (!is_on_map(newstep) || is_forbidden(newstep))  continue;
            
            // path up to the prev point + from prev point to current
            dist_t new_pure_dist = pd_pure_dist + pd.last_coord.dist_metric(newstep);
            dist_t new_total_dist = new_pure_dist + newstep.dist_metric(to);

            // TODO: check that new distance is shorter than in visited

            // if neighbor is not in heapq, add it
            if (visited.contains(newstep))  continue;

            std::vector<PathEntry> newpath = pd.path;
            newpath.push_back(PathEntry(dx, dy));
            PathDist newel = {.total_dist = new_total_dist, .last_coord = newstep, .path = newpath};
            heapq.emplace(newel);
        }
    }
    std::cerr << "FINISH TRAVERSAL\n";
}

int main(const int argc, char * const argv[])
{
    std::cerr << "Pathfinding\n";

    // Load test map
    BinMask mask("IDEAS/testmap.png");

    // std::pair from = {140, 780},
    //           to   = {545, 640};
    std::pair from = {545, 640},
              to   = {140, 780};
    
    // std::cout << "FROM HASH: " << Coord(from).hash() << std::endl;
    // std::cout << "TO HASH: " << Coord(to).hash() << std::endl; 

    auto path = find_path(mask, Coord(from), Coord(to));
    std::cerr << "Found path\n";
    auto [cur_x, cur_y] = from;
    auto i = 0;
    for (const auto &el : path) {
        auto [dx, dy] = el.to_coords();
        cur_x += dx;
        cur_y += dy;

        //std::cout << std::format("{:<3} [{}, {}]\t", i, cur_x, cur_y);
        //std::cout << el << std::endl;
        std::cout << cur_x << "\t" << cur_y << std::endl;
        i++;
    }
    
    return 0;
}