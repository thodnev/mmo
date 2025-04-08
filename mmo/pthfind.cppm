module;
// Pathfinding
// Damn algorithms are computational heavy.
// So keep everything as flat as possible, without nice (and sloppy)
// language features, to crank the hell out of performance.
// Hope it gets encapsulated by the upper hierarchy code.

#include <boost/heap/fibonacci_heap.hpp>

#include "macro.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

import types;
import utils;
import common;
export module pthfind;

export namespace pthfind {

// Core type aliases
using types::axis_t;        ///< Represents one coordinate axis: X or Y
using types::bbox_t;        ///< Relative coordinate axis inside bounding box
using types::dist_t;        ///< Represents distance between two points

// Derived types
// Coordinate pair (X, Y) - base for two concrete types
// using types::CoordBase;  // base not needed imported, use types::CoordBase

using types::Coord;         ///< Absolute coordinates pair (X, Y) on the whole map
using types::RelCoord;      ///< Relative coordinates pair [X, Y] inside bbox
// When coordinates are int16_t, RelCoord should get packed into one 32-bit value
// to ensure efficient storage and access.

/// Map object abstraction
using map_t = common::BinMask<axis_t>;      // @FIXME: probably BinMask shouldn't be templated

/// Type representing move step. This needs to be signed
using step_t = std::pair<bbox_t, bbox_t>;

/// Custom error used here
class LookupError : public std::range_error {
public:
    explicit LookupError(const std::string &message)
        : std::range_error(message) {}
};

/// Distance metric based on octile distance.
/// Octile distance is computed as:
/// dst = max(Δx, Δy) + (√2​−1)⋅min(Δx,Δy)
/// We approximate with integer arithmetic as:
/// dst*128 = 128 * max(Δx, Δy) + 53 * min(Δx,Δy)
/// Should give us 0.03% error
[[gnu::hot, gnu::always_inline]]
constexpr inline dist_t distance_octile(unsigned long ax, unsigned long ay,
                                        unsigned long bx, unsigned long by) noexcept
{
    // Don't use std::abs, std::min and std::max. Slow shit

    unsigned long dx = ax > bx ? ax - bx : bx - ax;   // higher bit overflows not controlled
    unsigned long dy = ay > by ? ay - by : by - ay;   // coord values must be smaller than that

    auto dst = 128 * (dx > dy ? dx : dy) + 53 * (dx < dy ? dx : dy);
    // return dst / 48;   
    // (!) Overfit without div. /2 and /4 in theory give 0.014% and 0.020% error
    // /16 is maximum 2**power. More, and straight/diagonal steps get indistinguishable
    return dst / 2;      
}

/// Wrapper encapsulating the concrete dist metric computation method
/// Hotter than any MILF next door, so keep it close.
/// We don't care what tag (absolute or relative kind) they have,
/// but we should always compare apples to apples.
template <typename B, typename Tag>
[[gnu::hot, gnu::always_inline]]
constexpr inline dist_t distance(const types::CoordBase<B, Tag> from, const types::CoordBase<B, Tag> to) noexcept
{
    return distance_octile(from.x, from.y, to.x, to.y);
}

/// Moving steps, all possible combinations
/*static */ const std::array<step_t, 8> steps = {
    // diagonals
    // straights
    step_t{ 1, 1},
    step_t{1, 0},

    step_t{ 1,  -1},
    step_t{ 0, -1},
    
    step_t{ -1, -1},
    step_t{ -1, 0},

    step_t{-1,  1},
    step_t{0, 1}
};

/// Utility function to compute array of distances for steps at compile-time
template<size_t size>
/*static */ constexpr auto __make_distarr(const decltype(steps) &steps) noexcept
{
    std::array<dist_t, size> res;
    const Coord base = {1, 1};
    for (decltype(size) i = 0; i < size; i++) {
        const auto [dx, dy] = steps[i];
        const Coord to = {static_cast<axis_t>(base.x + dx),
                          static_cast<axis_t>(base.y + dy)};
        res[i] = distance(base, to);
    }
    return res;
}

/// Precomputed distances for each step
/*static */ const auto steps_dist = __make_distarr<steps.size()>(steps);


/// Encapsulates map boundaries check
constexpr bool is_on_map(const map_t &map, const Coord point) noexcept
{
    // don't check for point.x >= 0 && point.y >= 0
    // since here we're dealing with unsigned types
    return point.x < map.width && point.y < map.height;
}

/// Encapsulates check whether map cell is occupied
[[gnu::hot, gnu::always_inline]]
constexpr inline bool is_forbidden(const map_t &map, const Coord point)
{
    return false == map.get_value(point.x, point.y);     // already taken
}

[[gnu::hot, gnu::always_inline]]
constexpr inline bool is_forbidden_raw(const map_t &map, const Coord point) noexcept
{
    return false == map.get_value_raw(point.x, point.y);     // already taken
}

// Utility function
constexpr void ensure_coord_valid(const map_t &map, const Coord &point)
{
    if (!is_on_map(map, point)) {
        throw LookupError(std::format(
            "Coord ({}, {}) does not belong to map {}x{} or forbidden",
            point.x, point.y, map.width, map.height));
    }
    if (is_forbidden(map, point)) {
        throw LookupError(std::format(
            "Coord ({}, {}) is forbidden",
            point.x, point.y));
    }
}


struct BBox {
    Coord base;     ///< absolute point (top leftmost) from which relative coords offset
    RelCoord most;  ///< relative coordinate defining bottom right boundary

    explicit BBox(const Coord &from,
                  const bbox_t bbox_width, const bbox_t bbox_height)
        : base{(axis_t)std::max((long)from.x - bbox_width, 0L),
               (axis_t)std::max((long)from.y - bbox_height, 0L)},
          most{static_cast<bbox_t>(bbox_width * 2 + 1),
               static_cast<bbox_t>(bbox_height * 2 + 1)}
        {}

    explicit BBox(const Coord &from, const bbox_t bbox_radius)
        : BBox(from, bbox_radius, bbox_radius) {}

    BBox(const BBox &) = delete;       //< Delete copy constructor
    BBox(BBox &&) = delete;            //< and move constructor

    [[gnu::hot, gnu::always_inline]]
    constexpr inline bool is_inside(const RelCoord &rel) const
    {
        return (rel.x <= most.x) && (rel.y <= most.y);
    }

    /// Computes index in a flat array for a relative coordinate.
    /// Rel coords have an interesting property of index always
    /// being upper bounded by: bbox_width * bbox_height
    [[gnu::hot, gnu::always_inline]]
    constexpr inline size_t get_index(const RelCoord &rel) const noexcept
    {
        // @TODO: efficiently check that we're not returning indices out of bonds
        return (size_t)rel.x + (size_t)rel.y * (size_t)most.x;
    }

    [[gnu::hot, gnu::always_inline]]
    constexpr inline const RelCoord to_relative(const Coord &coord) const noexcept
    {
        // @TODO: find an efficient way to check boundaries
        return {static_cast<bbox_t>(coord.x - base.x), 
                static_cast<bbox_t>(coord.y - base.y)};
    }

    [[gnu::hot, gnu::always_inline]]
    constexpr inline const Coord to_absolute(const RelCoord &coord) const noexcept
    {
        return {static_cast<bbox_t>(base.x + coord.x),
                static_cast<bbox_t>(base.y + coord.y)};
    }
};


/// This is what we store in visited list (aka closed set)
struct VisitedEntry {
    RelCoord came_from;
    dist_t pure_dist;

    constexpr inline bool is_empty() const
    {
        return came_from.is_zero() && pure_dist == 0;
    }
};

/// This gets stored in heapq (aka open set)
struct HeapEntry {
    /// memory is cheap, so store both distances
    dist_t total_dist;      ///< pure_dist + heurestic
    dist_t pure_dist;       ///< actual distance that we counted so far
    RelCoord coord;         ///< current relative coordinate

    /// Required for MinHeap comparisons
    // (!!) seems stupid boost::fibonacci_heap is MaxHeap, so invert this shit
    inline bool operator<(const HeapEntry &other) const {
        return total_dist > other.total_dist;
    }
};


constexpr std::vector<types::PathEntry> reconstruct_path(
    const RelCoord &path_finish,
    const RelCoord &path_start,
    const BBox &bbox,
    const std::vector<VisitedEntry> &visited)   noexcept
{
    LOG("Backtracing path ({}, {}) -> ({}, {})",
        path_finish.x, path_finish.y,path_start.x, path_start.y);
    
    utils::TimeIt _time_rec_path{};

    std::vector<types::PathEntry> path;
    path.reserve(bbox.get_index(bbox.most));

    auto cur_coord = path_finish;
    size_t idx;
    while (cur_coord != path_start) {
        idx = bbox.get_index(cur_coord);

        const auto from_coord = visited[idx].came_from;
        // if (cur_coord == path_start)  break;    // paranoia
        const auto dx = cur_coord.x - from_coord.x,
                   dy = cur_coord.y - from_coord.y;
        
        const types::PathEntry pe(dx, dy);
        path.emplace_back(pe);

        cur_coord = from_coord;
    }

    path.shrink_to_fit();       // free unused mem

    _time_rec_path.report_took("path reconstruction");

    return path;
}


constexpr void reconstruct_visited(const BBox &bbox, const std::vector<VisitedEntry> &visited,
                                   std::vector<Coord> &result)  noexcept
{
    utils::TimeIt _time_rec_visited{};

    const size_t maxidx = bbox.get_index(bbox.most);
    
    result.clear();
    result.reserve(maxidx);  // alloc with excess

    for (size_t idx = 0; idx < maxidx; idx++)
    {
        if (visited[idx].is_empty())  continue;
        const RelCoord rel = {.x = (bbox_t)(idx % (size_t)bbox.most.x),
                              .y = (bbox_t)(idx / (size_t)bbox.most.x)};
        result.emplace_back(bbox.to_absolute(rel));
    }

    result.shrink_to_fit();     // free unused mem

    _time_rec_visited.report_took("visited reconstruction");
}


/// A* pathfinding implementation
/// @param map     Map on which to search for a path
/// @param from    Absolute coordinates of the starting point on map
/// @param to      Absolute coordinates of the end (finish) point on map
/// @param radius  Squircle radius limiting the area `from` starting point,
///                in which the lookup is performed.
///                When set to 0 (default) - no limits apply and
///                the whole map is traversed.
/// @param set_visited (optional) When vector is passed, the visited set having
///                all the (absolute) coordinates explored will be exported there
/* export */ auto pathfind_astar(const map_t &map, const Coord from, const Coord to, const dist_t radius = 0,
                                 std::optional<std::reference_wrapper<std::vector<Coord>>> set_visited = std::nullopt)
{
    utils::TimeIt _time_init(true);

    // Ensure coordinates belong to map and aren't forbidden
    ensure_coord_valid(map, from);
    ensure_coord_valid(map, to);

    // Precompute maximum metric given the radius. This will be used in comparisons
    const auto maxdist = distance(Coord{0, 0}, Coord{(axis_t)(radius), 0});

    // Check that we're looking inside boundaries (if boundaries set)
    auto dist = distance(from, to);
    LOG("Original distance = {}, maxdist = {}", dist, maxdist);
    if (maxdist && dist > maxdist) {
        throw LookupError(std::format(
            "Dist {} FROM ({}, {}) TO ({}, {}) exceeds {} (lookup radius {})",
            dist, from.x, from.y, to.x, to.y, maxdist, radius
        ));
    }

    // recalculate bbox based on map boundaries
    // top left corner serves as a coordinate offset to map global <-> relative coords
    const BBox bbox(from, radius);

    if (USE_DEBUG) {
        [[maybe_unused]] const Coord bbox_base = bbox.base;
        // bottom right corner in relative coordinates
        [[maybe_unused]] const RelCoord bbox_most = bbox.most;
        // transform global coords -> coords relative to bbox
        [[maybe_unused]] const RelCoord bbox_from = bbox.to_relative(from);
        [[maybe_unused]] const RelCoord bbox_to = bbox.to_relative(to);
        // {(bbox_sgn_t)std::max((long)to.x - bbox_base.x, 0L),
        // (bbox_sgn_t)std::max((long)to.y - bbox_base.y, 0L)};

        LOG("Set bbox base to: {}", bbox_base);
        LOG("Bbox most boundary: {}", bbox_most);
        LOG("Transformed FROM {} -> {}", from, bbox_from);
        LOG("Transformed TO {} -> {}", to, bbox_to);
        LOG("Original distance: {}, transformed distance: {}",
            distance(from, to), distance(bbox_from, bbox_to));

        LOG("VisitedEntry size: {}, HeapEntry size: {}", sizeof(VisitedEntry), sizeof(HeapEntry));
    }

    std::vector<VisitedEntry> visited;
    visited.assign(bbox.get_index(bbox.most) + 1,
                   VisitedEntry{.came_from = {0, 0}, .pure_dist = 0});

    // AVX is good. But we need faster inits. So memset. Thug life
    //std::memset(visited.data(), 0, visited.size() * sizeof(visited[0]));
    //LOG("Memset size: {}, capacity: {}", visited.size(), visited.capacity());

    // Add FROM point to visited
    visited[bbox.get_index(bbox.to_relative(from))] = {
        .came_from = bbox.to_relative(from),
        .pure_dist = 0
    };

    // LOG("Visited vector size: {} KiB", (visited.size() * sizeof(visited[0])) / 1024);
    // Alignment paranoia
    LOG("Visited vector total size: {} KiB",
        ((uint8_t *)&(*(visited.end() - 1)) - (uint8_t *)&(*visited.begin())) / 1024);
    

    // Fuck slow STL data structures.
    // Use Boost Fibonacci heap until we find something better
    boost::heap::fibonacci_heap<HeapEntry> heapq;
    // put the starting element
    heapq.push(HeapEntry{.total_dist = std::numeric_limits<dist_t>::max(),
                         .pure_dist = 0,
                         .coord = bbox.to_relative(from)
                        });

    const auto bboxed_to = bbox.to_relative(to);
    [[maybe_unused]] const auto bboxed_from = bbox.to_relative(from);
    
    // start traversal
    _time_init.report_took("initialization");
    utils::TimeIt _time_lookup{};

    while (! heapq.empty()) {
        // dequeue lowest element
        const auto el = heapq.top();
        heapq.pop();        // remove from heap

        // check whether destination reached
        if (el.coord == bboxed_to) {            // found
            _time_lookup.report_took("map traversal");
            LOG("FOUND PATH");

            if (set_visited) {
                reconstruct_visited(bbox, visited, *set_visited);
            }

            return reconstruct_path(
                el.coord,
                bbox.to_relative(from), bbox, visited
            );     // @FIXME
        }

        // check neighbors
        for (size_t i = 0; i < steps.size(); i++) {
            const auto [dx, dy] = steps[i];          // should be more efficient
            const auto step_dist = steps_dist[i];    // than unpacking tuples of tuples

            const RelCoord newcoord = { .x = static_cast<bbox_t>(el.coord.x + dx), 
                                        .y = static_cast<bbox_t>(el.coord.y + dy) };
            
            // @TODO: find more optimal way for this check
            if (newcoord.x < 0 || newcoord.y < 0) {
                continue;       // rel coords can't be negative
            }

            const auto new_pure_dist = el.pure_dist + step_dist;

            // Check that we're inside bbox first before checking visited. This way
            // the check comes for free. Otherwise we risk indexing out of bonds

            // @TODO: check dist_metric
            // Note: is_forbidden_raw() is safe here as we're already ensuring map
            //       boundaries with bbox. This allows to avoid repetitive check
            if (//distance(bboxed_from, newcoord) > maxdist   ||
                !bbox.is_inside(newcoord)
                || is_forbidden_raw(map, bbox.to_absolute(newcoord))) {

                continue;
            }

            const auto idx = bbox.get_index(newcoord);
            if (!visited[idx].is_empty() && new_pure_dist >= visited[idx].pure_dist) {
                continue;       // don't revisit if not more optimal
            }

            // enqueue if found new or more optimal point
            visited[idx].pure_dist = new_pure_dist;
            visited[idx].came_from = el.coord;
            
            const HeapEntry newel = {
                // if we use new_pure + mul * distance here,
                // with mul -> 0 we're getting closer to Dijkstra
                // with mul > 1 we're getting closer to greedy breadth-first
                .total_dist = new_pure_dist + distance(newcoord, bboxed_to),
                .pure_dist = new_pure_dist,
                .coord = newcoord
            };

            heapq.push(newel);
        }
    }
    
    LOG("PATH NOT FOUND");

    if (set_visited) {
        reconstruct_visited(bbox, visited, *set_visited);
    }

    return std::vector<types::PathEntry>();     // @FIXME
}


// export namespace pthfind {
//     using ::pathfind_astar;
// }

}   // namespace
