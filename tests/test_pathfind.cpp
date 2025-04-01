#include <cmath>
#include <compare>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
//#include <queue>
#include <set>
#include <vector>

#include <chrono>

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
        std::cerr << "Pure distance: " << from.dist_metric(to) << std::endl;

    // @TODO: make it Coord methods
    auto is_on_map = [&map](const Coord<U> &coord) constexpr {
            return coord.x < map.width && coord.y < map.height && coord.x >= 0 && coord.y >= 0;
    };

    auto ensure_on_map = [&map, &is_on_map](const Coord<U> &coord) constexpr {
        if (! is_on_map(coord)) {
            throw std::range_error(std::format(
                "Coord {} does not belong to map {}x{}",
                (std::string)coord, map.width, map.height));
        }
    };

    auto is_forbidden = [&map](const Coord<U> &coord) constexpr {
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
        Coord<U> came_from;

        // std::strong_ordering operator<=>(const PathDist &other) const
        // {
        //     return total_dist <=> other.total_dist;
        // }

        // Ensures paths with the same cost (f(n)) but different destinations
        // (last_coord) are treated as unique elements in std::set
        bool operator<(const PathDist &other) const {
            bool coords_equal = (last_coord == other.last_coord);
            return ((total_dist < other.total_dist) 
                || ((total_dist == other.total_dist) && !coords_equal));
        }
    };

    // moving steps, all possible combinations
    //const auto steps = PathEntry::_steps_table;
    
    std::vector< std::pair<decltype(PathEntry::_steps_table)::value_type,  dist_t>>  steps;
    steps.reserve(PathEntry::_steps_table.size());
    // build dist_metric deltas for each step
    {
        using step_t = decltype(steps)::value_type;
        Coord<int> zero = {0, 0};
        for (const auto &move : PathEntry::_steps_table) {
            Coord<int> delta = {(int)(std::get<0>(move)), (int)(std::get<1>(move))};
            auto dist = zero.dist_metric(delta);
            step_t el = {move, dist};
            steps.emplace_back(el);
        }
    }
    
    // heap queue with .top() always returning the PathDist with smallest distance
    // std::priority_queue<PathDist, std::vector<PathDist>, std::greater<PathDist>>
    //     heapq;

    // std::set always stores its elements sorted by key ascending
    // this way heapq.begin() always points to the element with the lowest key
    std::set<PathDist> heapq;
    
    heapq.emplace(PathDist{.total_dist = from.dist_metric(to), .last_coord = from, .came_from = from});

    // stored coords we already visited of a form:
    // {current: (distance, came_from)}
    struct VisitedEl {
        dist_t dist;
        Coord<U> came_from = {0,0};
    };
    std::unordered_map<Coord<U>, VisitedEl> visited;

    auto reconstruct_path = [&](const PathDist last) {
        std::vector<PathEntry> path;
        auto cur = last.came_from;
        while (cur != from) {
            auto prev = visited[cur].came_from;

            PathEntry pe(cur.x - prev.x,
                         cur.y - prev.y);
            path.emplace_back(pe);

            cur = prev;
        }
        return path;
    };

    // traverse the map
    while (! heapq.empty()) {
        auto pd = *heapq.begin();
        // std::cout << (std::string)pd.last_coord << std::endl;
        // std::cin.get();

        // when node is the goal, return the found path
        if (pd.last_coord == to)  return reconstruct_path(pd);
        // {
        //     std::vector<Coord<U>> res(visited.begin(), visited.end());
        //     return res;
        // }

        // move the node to visited
        visited[pd.last_coord] = {pd.total_dist, pd.came_from}; // TODO: fix price;
        heapq.erase(heapq.begin());

        auto pd_eval_dist = pd.last_coord.dist_metric(to);
        auto pd_pure_dist = pd.total_dist - pd_eval_dist;

        // for each neighbor of the current node
        for (const auto &[_pair, dist_delta]: steps) {
            const auto [dx, dy] = _pair;
            Coord<U> newstep = {static_cast<U>(pd.last_coord.x + dx),
                                static_cast<U>(pd.last_coord.y + dy)};
            
            // ensure we can go here
            if (!is_on_map(newstep) || is_forbidden(newstep))  continue;
            
            // path up to the prev point + from prev point to current
            dist_t new_pure_dist = pd_pure_dist + dist_delta;
            dist_t new_total_dist = new_pure_dist + newstep.dist_metric(to);

            // TODO: check that new distance is shorter than in visited

            // if neighbor is not in heapq, add it
            if (visited.contains(newstep))  continue;

            PathDist newel = {.total_dist = new_total_dist, .last_coord = newstep, .came_from = pd.last_coord};
            heapq.emplace(newel);
        }
    }
    std::cerr << "FINISH TRAVERSAL\n";
}

template<typename U>
void mapout(const BinMask<U> &mask)
{
    for (size_t y = 0; y < mask.height; y++) {
        for (size_t x = 0; x < mask.width; x++) {
            auto val = mask.get_value(x, y);
            std::cout << (val ? "  " : "##");
        }
        std::cout << std::endl;
    }
}

int main(const int argc, char * const argv[])
{
    std::cerr << "Pathfinding\n";

    // Load test map
    BinMask mask("IDEAS/testmap.png");
    //mapout(mask);

    // std::pair from = {79, 145},
    //           to   = {69, 110};
    std::pair from = {477, 862},
              to   = {731, 154};
    // std::pair from = {545, 640},
    // to   = {140, 780};
    
    // std::cout << "FROM HASH: " << Coord(from).hash() << std::endl;
    // std::cout << "TO HASH: " << Coord(to).hash() << std::endl; 

    auto tstart = std::chrono::high_resolution_clock::now();

    auto path = find_path(mask, Coord(from), Coord(to));

    auto tend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> took = tend - tstart;
    std::cerr << std::format("Found path in {:.3f} s\n", took.count());
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
    // for (const auto &el : path) {
    //     std::cout << el.x << "\t" << el.y << std::endl;
    // }
    
    return 0;
}