#ifndef DOWNWELL_CORE_SOLID_CELL_GRID_H
#define DOWNWELL_CORE_SOLID_CELL_GRID_H
#include <cstdint>

namespace downwell {

// Aligned 16x16 occupancy cells borrowed from a backend's terrain index, for
// the same answers as its point query without a call per sample. A
// breakable cell is solid only while its box is live, which the owner of
// the boxes decides from the cell's top-left offset coordinates.
struct SolidCellGrid {
    const std::uint8_t* cells = nullptr;
    int originX = 0, originY = 0; // added to an offset pixel before >>4
    unsigned columnShift = 0; // 1<<columnShift cells per row
    unsigned width = 0, height = 0; // in pixels: 16<<columnShift, 16 per row
    std::uint8_t solid = 0, breakable = 0;
    bool valid() const { return cells != nullptr; }
};

// Point and four-corner queries on a grid. Breakable(x, y) receives a cell's
// top-left in the same offset coordinates as the samples.
template<class Breakable>
struct SolidCells {
    const SolidCellGrid* grid;
    Breakable breakable;
    // One cell's value at a sample already offset by the grid origin.
    bool occupied(std::uint8_t value, unsigned sx, unsigned sy) const {
        return (value & grid->solid) || ((value & grid->breakable) &&
            breakable(int(sx & ~15u) - grid->originX, int(sy & ~15u) - grid->originY));
    }
    const std::uint8_t* row(unsigned sy) const { return grid->cells + ((sy >> 4) << grid->columnShift); }
    bool operator()(int x, int y) const {
        const unsigned sx = unsigned(x + grid->originX), sy = unsigned(y + grid->originY);
        return sx < grid->width && sy < grid->height && occupied(row(sy)[sx >> 4], sx, sy);
    }
};
// The union of the four corner samples, in their order: each distinct cell
// is read once, each row's base computed once.
template<class Breakable>
bool solidCorners(const SolidCells<Breakable>& solid, int left, int top, int right, int bottom) {
    const SolidCellGrid& grid = *solid.grid;
    const unsigned x0 = unsigned(left + grid.originX), x1 = unsigned(right + grid.originX);
    const unsigned y0 = unsigned(top + grid.originY), y1 = unsigned(bottom + grid.originY);
    const bool first = x0 < grid.width, second = (x0 >> 4) != (x1 >> 4) && x1 < grid.width;
    const auto across = [&](unsigned sy) {
        if (sy >= grid.height) return false;
        const std::uint8_t* cells = solid.row(sy);
        return (first && solid.occupied(cells[x0 >> 4], x0, sy)) ||
               (second && solid.occupied(cells[x1 >> 4], x1, sy));
    };
    return across(y0) || ((y0 >> 4) != (y1 >> 4) && across(y1));
}

} // namespace downwell
#endif
