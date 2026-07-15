#ifndef ENGINE_COLLISIONGRID_H
#define ENGINE_COLLISIONGRID_H

#include <vector>
#include <unordered_map>
#include "../Triangle.h"
#include "../math/Vec3D.h"

struct GridCell {
    int x, y, z;
    bool operator==(const GridCell &o) const { return x == o.x && y == o.y && z == o.z; }
};

struct GridCellHash {
    size_t operator()(const GridCell &c) const {
        return static_cast<size_t>(c.x) * 73856093 ^
               static_cast<size_t>(c.y) * 19349663 ^
               static_cast<size_t>(c.z) * 83492791;
    }
};

class CollisionGrid {
public:
    void clear();
    void addTriangle(const Triangle &tri, double cellSize = 5.0);
    std::vector<const Triangle*> querySphere(const Vec3D &center, double radius) const;

private:
    double _cellSize = 5.0;
    std::unordered_map<GridCell, std::vector<Triangle>, GridCellHash> _cells;
};

#endif
