#include <cmath>
#include "CollisionGrid.h"

void CollisionGrid::clear() {
    _cells.clear();
}

void CollisionGrid::addTriangle(const Triangle &tri, double cellSize) {
    _cellSize = cellSize;
    Vec3D min(1e9,1e9,1e9), max(-1e9,-1e9,-1e9);
    for (int i = 0; i < 3; i++) {
        Vec3D v(tri[i].x(), tri[i].y(), tri[i].z());
        if (v.x()<min.x()) min=Vec3D(v.x(),min.y(),min.z());
        if (v.y()<min.y()) min=Vec3D(min.x(),v.y(),min.z());
        if (v.z()<min.z()) min=Vec3D(min.x(),min.y(),v.z());
        if (v.x()>max.x()) max=Vec3D(v.x(),max.y(),max.z());
        if (v.y()>max.y()) max=Vec3D(max.x(),v.y(),max.z());
        if (v.z()>max.z()) max=Vec3D(max.x(),max.y(),v.z());
    }
    int minX = static_cast<int>(floor(min.x() / cellSize));
    int minY = static_cast<int>(floor(min.y() / cellSize));
    int minZ = static_cast<int>(floor(min.z() / cellSize));
    int maxX = static_cast<int>(floor(max.x() / cellSize));
    int maxY = static_cast<int>(floor(max.y() / cellSize));
    int maxZ = static_cast<int>(floor(max.z() / cellSize));
    for (int gx = minX; gx <= maxX; gx++)
        for (int gy = minY; gy <= maxY; gy++)
            for (int gz = minZ; gz <= maxZ; gz++)
                _cells[{gx, gy, gz}].push_back(tri);
}

std::vector<const Triangle*> CollisionGrid::querySphere(const Vec3D &center, double radius) const {
    std::vector<const Triangle*> result;
    int minX = static_cast<int>(floor((center.x() - radius) / _cellSize));
    int minY = static_cast<int>(floor((center.y() - radius) / _cellSize));
    int minZ = static_cast<int>(floor((center.z() - radius) / _cellSize));
    int maxX = static_cast<int>(floor((center.x() + radius) / _cellSize));
    int maxY = static_cast<int>(floor((center.y() + radius) / _cellSize));
    int maxZ = static_cast<int>(floor((center.z() + radius) / _cellSize));
    for (int gx = minX; gx <= maxX; gx++)
        for (int gy = minY; gy <= maxY; gy++)
            for (int gz = minZ; gz <= maxZ; gz++) {
                auto it = _cells.find({gx, gy, gz});
                if (it != _cells.end())
                    for (const auto &tri : it->second)
                        result.push_back(&tri);
            }
    return result;
}
