module;
// Pathfinding
// Damn algorithms are computational heavy.
// So keep everything as flat as possible, without nice (and sloppy)
// language features, to crank the hell out of performance.
// Hope it gets encapsulated by the upper hierarchy code.

#include <cstdint>
#include <format>
#include <type_traits>
#include <utility>
#include <vector>

#include "macro.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

#include <boost/heap/fibonacci_heap.hpp>

export module pthfind;
import types;
import common;

export namespace pthfind {
// Core type aliases
/// Represents one coordinate: X or Y
using axis_t = unsigned int;

/// In pathfinding we are using relative coordinates inside bounding box
/// This will allow to have shorter types and save some space.
/// Relative coordinate inside bbox
using bbox_t = uint16_t;

/// Used to represent distance between two points
/// Theoretically, octile dist_metric as implemented below
/// gives max value of 181/2. Thus 32-bit value should be enough
/// to store distances for fields up to 6888x6888
/// This also needs not to be huge to save memory in vector tables
/// (of dist_t x visited size)
using dist_t = unsigned int;


// Derived types

/// The same width as bbox_t, but signed
using bbox_sgn_t = std::make_signed_t<bbox_t>;

/// Coordinate pair (X, Y) - base for two concrete types
template <typename T>
struct CoordBase {
    using type = T;
    type x;
    type y;

    constexpr inline bool is_zero() const
    {
        return !x && !y;
    }

    constexpr inline bool operator==(const CoordBase<T> &other) const
    {
        return x == other.x && y == other.y;
    }
};

/// Represents coordinates on the whole map
using Coord = CoordBase<axis_t>;
static_assert(sizeof(Coord) == 2 * sizeof(axis_t),
              "Coord struct not packed efficiently");

/// When coordinates are uint16_t, it should get packed into one 32-bit value
/// to ensure efficient storage and access.
using RelCoord = CoordBase<bbox_sgn_t>;
static_assert(sizeof(RelCoord) == 2 * sizeof(bbox_sgn_t),
              "RelCoord struct not packed efficiently");


/// Map object abstraction
using map_t = common::BinMask<axis_t>;      // @FIXME: probably BinMask shouldn't be templated

/// Type representing move step. This needs to be signed
using step_t = std::pair<bbox_sgn_t, bbox_sgn_t>;


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
    // unsigned long dx = std::abs((long)this->x - (long)other.x);
    // unsigned long dy = std::abs((long)this->y - (long)other.y);
    // auto dst = 128 * std::max(dx, dy) + 53 * std::min(dx, dy);
    // return dst / 2;
    unsigned long dx = ax > bx ? ax - bx : bx - ax;   // higher bit overflows not controlled
    unsigned long dy = ay > by ? ay - by : by - ay;   // coord values must be smaller than that

    auto dst = 128 * (dx > dy ? dx : dy) + 53 * (dx < dy ? dx : dy);
    // dunno why, but result/2 it is faster. Save one asm instr by now
    return dst / 64;     
}

/// Wrapper encapsulating the concrete dist metric computation method
/// Hotter than any MILF next door, so keep it close
template <typename B>
[[gnu::hot, gnu::always_inline]]
constexpr inline dist_t distance(const CoordBase<B> from, const CoordBase<B> to) noexcept
{
    return distance_octile(from.x, from.y, to.x, to.y);
}

/// Encapsulates map boundaries check
constexpr bool is_on_map(const map_t &map, const Coord point) noexcept
{
    // don't check for point.x >= 0 && point.y >= 0
    // since here we're dealing with unsigned types
    return point.x < map.width && point.y < map.height;
}

/// Encapsulates check whether map cell is occupied
[[gnu::hot]] constexpr bool is_forbidden(const map_t &map, const Coord point) noexcept
{
    return false == map.get_value(point.x, point.y);     // already taken
}

/// Custom error used here
class LookupError : public std::range_error {
public:
    explicit LookupError(const std::string &message)
        : std::range_error(message) {}
};

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


struct BBox {
    Coord base;     ///< absolute point (top leftmost) from which relative coords offset
    RelCoord most;  ///< relative coordinate defining bottom right boundary

    explicit BBox(const Coord &from,
                  const bbox_t bbox_width, const bbox_t bbox_height)
        : base{(axis_t)std::max((long)from.x - bbox_width, 0L),
               (axis_t)std::max((long)from.y - bbox_height, 0L)},
          most{static_cast<bbox_sgn_t>(bbox_width * 2 + 1),
               static_cast<bbox_sgn_t>(bbox_height * 2 + 1)}
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
    constexpr inline size_t get_index(const RelCoord &rel) const
    {
        // @TODO: efficiently check that we're not returning indices out of bonds
        return (size_t)rel.x + (size_t)rel.y * (size_t)most.x;
    }

    [[gnu::hot, gnu::always_inline]]
    constexpr inline const RelCoord to_relative(const Coord &coord) const
    {
        // @TODO: find an efficient way to check boundaries
        return {static_cast<bbox_sgn_t>(coord.x - base.x), 
                static_cast<bbox_sgn_t>(coord.y - base.y)};
    }

    [[gnu::hot, gnu::always_inline]]
    constexpr inline const Coord to_absolute(const RelCoord &coord) const
    {
        return {base.x + coord.x,
                base.y + coord.y};
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
    const std::vector<VisitedEntry> &visited)
{
    LOG("Backtracing path ({}, {}) -> ({}, {})",
        path_finish.x, path_finish.y,path_start.x, path_start.y);
    
    utils::TimeIt _time_reconstr{};

    std::vector<types::PathEntry> path;
    path.reserve(bbox.get_index(bbox.most));

    auto cur_coord = path_finish;
    size_t idx;
    while (cur_coord != path_start) {
        idx = bbox.get_index(cur_coord);

        const auto from_coord = visited[idx].came_from;
        if (cur_coord == path_start)  break;
        const auto dx = cur_coord.x - from_coord.x,
                   dy = cur_coord.y - from_coord.y;
        
        const types::PathEntry pe(dx, dy);
        path.emplace_back(pe);

        cur_coord = from_coord;
    }

    _time_reconstr.report_took("path reconstruction");

    return path;
}


/// A* pathfinding implementation
/// @param radius   Squircle radius limiting the area `from` starting point,
///                 in which the lookup is performed.
///                 When set to 0 (default) - no limits apply and
///                 the whole map is traversed.
/* export */ auto pathfind_astar(const map_t &map, const Coord from, const Coord to, const dist_t radius = 0)
{
    utils::TimeIt _time_init(true);

    // Ensure coordinates belong to map and aren't forbidden
    ensure_coord_valid(map, from);
    ensure_coord_valid(map, to);

    // Precompute maximum metric given the radius. This will be used in comparisons
    const auto maxdist = distance(Coord{0, 0}, Coord{(dist_t)(radius), 0});

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

    const Coord bbox_base = bbox.base;
    // bottom right corner in relative coordinates
    const RelCoord bbox_most = bbox.most;
    // transform global coords -> coords relative to bbox
    const RelCoord bbox_from = bbox.to_relative(from);
    const RelCoord bbox_to = bbox.to_relative(to);
    // {(bbox_sgn_t)std::max((long)to.x - bbox_base.x, 0L),
    // (bbox_sgn_t)std::max((long)to.y - bbox_base.y, 0L)};

    LOG("Set bbox base to: ({}, {})", bbox_base.x, bbox_base.y);
    LOG("Bbox most boundary: ({}, {})", bbox_most.x, bbox_most.y);
    LOG("Transformed FROM ({}, {}) -> ({}, {})", from.x, from.y, bbox_from.x, bbox_from.y);
    LOG("Transformed TO ({}, {}) -> ({}, {})", to.x, to.y, bbox_to.x, bbox_to.y);
    LOG("Original distance: {}, transformed distance: {}",
        distance(from, to), distance(bbox_from, bbox_to));

    LOG("VisitedEntry size: {}, HeapEntry size: {}", sizeof(VisitedEntry), sizeof(HeapEntry));
    
    std::vector<VisitedEntry> visited;
    visited.assign(bbox.get_index(bbox.most) + 1,
                   VisitedEntry{.came_from = {0, 0}, .pure_dist = 0});
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
    heapq.push(HeapEntry{.coord = bbox.to_relative(from),
                         .total_dist = std::numeric_limits<dist_t>::max(),
                         .pure_dist = 0});

    const auto bboxed_to = bbox.to_relative(to);
    
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
            return reconstruct_path(
                el.coord,
                bbox.to_relative(from), bbox, visited
            );     // @FIXME
        }

        // check neighbors
        for (size_t i = 0; i < steps.size(); i++) {
            const auto [dx, dy] = steps[i];          // should be more efficient
            const auto step_dist = steps_dist[i];    // than unpacking tuples of tuples

            const RelCoord newcoord = { .x = static_cast<bbox_sgn_t>(el.coord.x + dx), 
                                        .y = static_cast<bbox_sgn_t>(el.coord.y + dy) };
            
            // @TODO: find more optimal way for this check
            if (newcoord.x < 0 || newcoord.y < 0) {
                continue;       // rel coords can't be negative
            }

            const auto new_pure_dist = el.pure_dist + step_dist;

            // @TODO: play with the order of checks

            // @TODO: check dist_metric
            if (!bbox.is_inside(newcoord) || is_forbidden(map, bbox.to_absolute(newcoord))) {
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
                .total_dist = new_pure_dist + distance(newcoord, bboxed_to),
                .pure_dist = new_pure_dist,
                .coord = newcoord
            };

            heapq.push(newel);
        }
    }
    
    LOG("PATH NOT FOUND");
    return std::vector<types::PathEntry>();     // @FIXME
}


// export namespace pthfind {
//     using ::pathfind_astar;
// }

}   // namespace