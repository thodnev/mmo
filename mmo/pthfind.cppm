module;
// Pathfinding
// Damn algorithms are computational heavy.
// So keep everything as flat as possible, without nice (and sloppy)
// language features, to crank the hell out of performance.
// Hope it gets encapsulated by the upper hierarchy code.

#include <array>
#include <format>
#include <stdexcept>
#include <utility>
#include <vector>

export module pthfind;
import common;

// Core type aliases
/// Represents one coordinate: X or Y
using axis_t = unsigned int;

/// Used to represent distance between two points
/// Theoretically, octile dist_metric as implemented below
/// gives max value of 181/2. Thus 32-bit value should be enough
/// to store distances for fields up to 6888x6888
/// This also needs not to be huge to save memory in vector tables
/// (of dist_t x visited size)
using dist_t = unsigned int;


// Derived types

/// Coordinate pair (X, Y)
/// When coordinates are uint, this should get packed into one 64-bit value
/// to ensure efficient storage and access.
struct Coord {
    axis_t x;
    axis_t y;
};
static_assert(sizeof(Coord) == 2 * sizeof(axis_t),
              "Coord struct not packed efficiently");

/// Map object abstraction
using map_t = common::BinMask<axis_t>;      // @FIXME: probably BinMask shouldn't be templated

/// Type representing move step. This needs to be signed
using step_t = std::pair<int, int>;


/// Distance metric based on octile distance.
/// Octile distance is computed as:
/// dst = max(Δx, Δy) + (√2​−1)⋅min(Δx,Δy)
/// We approximate with integer arithmetic as:
/// dst*128 = 128 * max(Δx, Δy) + 53 * min(Δx,Δy)
/// Should give us 0.03% error
[[gnu::hot]] constexpr dist_t distance_octile(unsigned long ax, unsigned long ay,
                                 unsigned long bx, unsigned long by) noexcept
{
    // Don't use std::abs, std::min and std::max. Slow shit
    // unsigned long dx = std::abs((long)this->x - (long)other.x);
    // unsigned long dy = std::abs((long)this->y - (long)other.y);
    // auto dst = 128 * std::max(dx, dy) + 53 * std::min(dx, dy);
    // return dst / 2; 

    unsigned long dx = ax - ay;   // higher bit overflows not controlled
    unsigned long dy = bx - by;   // coord values must be smaller than that

    auto dst = 128 * (dx > dy ? dx : dy) + 53 * (dx > dy ? dy : dx);
    // dunno why, but result/2 it is faster. Save one asm instr by now
    return dst;     
}

/// Wrapper encapsulating the concrete dist metric computation method
/// Hotter than any MILF next door, so keep it close
[[gnu::hot]] dist_t distance(const Coord from, const Coord to) noexcept
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
class LookupError : public std::range_error {};

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
static const std::array<step_t, 8> steps = {
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
static constexpr auto __make_distarr(const decltype(steps) &steps) noexcept
{
    std::array<dist_t, size> res;
    const Coord base = {1, 1};
    for (auto i = 0; i < size; i++) {
        const auto [dx, dy] = steps[i];
        const Coord to = {static_cast<axis_t>(base.x + dx),
                          static_cast<axis_t>(base.y + dy)};
        res[i] = distance(base, to);
    }
    return res;
}

/// Precomputed distances for each step
static const auto steps_dist = __make_distarr<steps.size()>(steps);



/// A* pathfinding implementation
/// @param radius   Squircle radius limiting the area `from` starting point,
///                 in which the lookup is performed.
///                 When set to 0 (default) - no limits apply and
///                 the whole map is traversed.
auto pathfind_astar(const map_t &map, const Coord from, const Coord to, const dist_t radius = 0)
{

    // Ensure coordinates belong to map and aren't forbidden
    ensure_coord_valid(map, from);
    ensure_coord_valid(map, to);

    // Precompute maximum metric given the radius. This will be used in comparisons
    const auto maxdist = distance(Coord{0, 0}, Coord{static_cast<dist_t>(radius), 0});

    // Check that we're looking inside boundaries (if boundaries set)
    if (maxdist && distance(from, to) > maxdist) {
        throw LookupError(std::format(
            "Dist between from ({}, {}) and to ({}, {}) exceeds lookup radius {}",
            from.x, from.y, to.x, to.y, radius
        ));
    }

    // @TODO: recalculate bbox based on map boundaries
}