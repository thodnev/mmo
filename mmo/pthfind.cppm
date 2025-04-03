module;
// Pathfinding
// Damn algorithms are computational heavy.
// So keep everything as flat as possible, without nice (and sloppy)
// language features, to crank the hell out of performance.
// Hope it gets encapsulated by the upper hierarchy code.
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

using map_t = common::BinMask<axis_t>;      // @FIXME: probably BinMask shouldn't be templated

/// Distance metric based on octile distance.
/// Octile distance is computed as:
/// dst = max(Δx, Δy) + (√2​−1)⋅min(Δx,Δy)
/// We approximate with integer arithmetic as:
/// dst*128 = 128 * max(Δx, Δy) + 53 * min(Δx,Δy)
/// Should give us 0.03% error
constexpr dist_t distance_octile(unsigned long ax, unsigned long ay,
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
dist_t distance(const Coord from, const Coord to) noexcept
{
    return distance_octile(from.x, from.y, to.x, to.y);
}




