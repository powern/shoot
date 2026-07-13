#ifndef ENGINE_WORLD_H
#define ENGINE_WORLD_H

#include <map>
#include <unordered_map>
#include <set>
#include <vector>

#include "Camera.h"
#include "io/Screen.h"
#include "physics/RigidBody.h"

struct IntersectionInformation final {
    const Vec3D pointOfIntersection;
    const double distanceToObject;
    const Triangle intersectedTriangle;
    const ObjectNameTag objectName;
    const std::shared_ptr<RigidBody> obj;
    const bool intersected;
};

struct ContactPair final {
    ObjectNameTag a;
    ObjectNameTag b;
    double lastContactTime = 0.0;
    Vec3D accumulatedImpulse{0, 0, 0};
};

class World final {
private:
    std::map<ObjectNameTag, std::shared_ptr<RigidBody>> _objects;

    // Spatial grid broadphase
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
    std::unordered_map<GridCell, std::vector<ObjectNameTag>, GridCellHash> _grid;

    // Contact caching for warm starting
    std::vector<ContactPair> _contacts;
    double _physicsTime = 0.0;

    void _buildBroadphase();
    void _checkCollisionPair(const ObjectNameTag &a, const ObjectNameTag &b);
    void checkCollision(const ObjectNameTag &tag);
public:
    World() = default;

    void update();

    std::shared_ptr<RigidBody> addBody(std::shared_ptr<RigidBody> mesh);
    std::shared_ptr<RigidBody> body(const ObjectNameTag &tag);
    void removeBody(const ObjectNameTag &tag);
    std::shared_ptr<RigidBody> loadBody(const ObjectNameTag &tag, const std::string &filename, const Vec3D &scale = Vec3D{1, 1, 1});
    void loadMap(const std::string &filename, const Vec3D &scale = Vec3D{1, 1, 1});

    // std::string skipTags is a string that consist of all objects we want to skip in ray casting
    IntersectionInformation rayCast(const Vec3D &from, const Vec3D &to, const std::string &skipTags = "");

    std::map<ObjectNameTag, std::shared_ptr<RigidBody>>::iterator begin() { return _objects.begin(); }
    std::map<ObjectNameTag, std::shared_ptr<RigidBody>>::iterator end() { return _objects.end(); }

    void stepPhysics(double dt);
};


#endif //INC_3DZAVR_WORLD_H
