#include <sstream>
#include <cmath>
#include <unordered_set>

#include "World.h"
#include "utils/Log.h"
#include "math/Plane.h"
#include "utils/ResourceManager.h"
#include "utils/Time.h"

using namespace std;

std::shared_ptr<RigidBody> World::addBody(std::shared_ptr<RigidBody> body) {
    _objects.emplace(body->name(), body);
    Log::log("World::addBody(): inserted body '" + body->name().str() + "' with " +
             std::to_string(_objects[body->name()]->triangles().size()) + " tris.");
    return _objects[body->name()];
}

std::shared_ptr<RigidBody> World::loadBody(const ObjectNameTag &tag, const string &filename, const Vec3D &scale) {
    _objects.emplace(tag, std::make_shared<RigidBody>(tag, filename, scale));
    Log::log("World::loadBody(): inserted body from " + filename + " with title '" + tag.str() + "' with " +
             std::to_string(_objects[tag]->triangles().size()) + " tris.");
    return _objects[tag];
}

IntersectionInformation World::rayCast(const Vec3D &from, const Vec3D &to, const std::string &skipTags) {

    // make vector of tags, that we are going to escape
    vector<std::string> tagsToSkip;
    stringstream s(skipTags);
    std::string t;
    while (s >> t) {
        tagsToSkip.push_back(t);
    }

    bool intersected = false;
    Vec3D point{};
    Triangle triangle;
    std::string bodyName;
    double minDistance = Consts::RAY_CAST_MAX_DISTANCE;
    std::shared_ptr<RigidBody> intersectedBody = nullptr;

    for (auto&[name, body]  : _objects) {

        bool escapeThisBody = false;
        for (auto &escapeTag : tagsToSkip) {
            if (name.contains(ObjectNameTag(escapeTag))) {
                escapeThisBody = true;
                break;
            }
        }
        if (escapeThisBody) {
            continue;
        }

        Matrix4x4 model = body->model();
        Matrix4x4 invModel = body->invModel();

        Vec3D v = (to - from).normalized();
        Vec3D v_model = invModel*v;
        Vec3D from_model = invModel*(from - body->position());
        Vec3D to_model = invModel*(to - body->position());


        for (auto &tri : body->triangles()) {

            if(tri.norm().dot(v_model) > 0) {
                continue;
            }

            auto intersection = Plane(tri).intersection(from_model, to_model);

            if (intersection.second > 0 && tri.isPointInside(intersection.first)) {

                Triangle globalTriangle(model * tri[0], model * tri[1], model * tri[2], tri.color());
                auto globalIntersection = Plane(globalTriangle).intersection(from, to);
                double globalDistance = (globalIntersection.first - from).abs();

                if(globalDistance < minDistance) {
                    minDistance = globalDistance;
                    point = globalIntersection.first;
                    triangle = globalTriangle;
                    bodyName = name.str();
                    intersected = true;
                    intersectedBody = body;
                }
            }
        }
    }

    return IntersectionInformation{point, sqrt(minDistance), triangle, ObjectNameTag(bodyName), intersectedBody, intersected};
}

void World::loadMap(const std::string &filename, const Vec3D &scale, const Matrix4x4 &postTransform) {
    auto objs = ResourceManager::loadObjects(filename);
    for (auto &i : objs) {
        std::shared_ptr<RigidBody> obj = std::make_shared<RigidBody>(*i, false);
        addBody(obj);
        obj->scale(scale);
        obj->transform(postTransform);
        obj->setStatic(true);
    }
}

void World::removeBody(const ObjectNameTag &tag) {
    if (_objects.erase(tag) > 0) {
        Log::log("World::removeBody(): removed body '" + tag.str() + "'");
    } else {
        Log::log("World::removeBody(): cannot remove body '" + tag.str() + "': body does not exist.");
    }
}

void World::_buildBroadphase() {
    _grid.clear();

    for (auto &[name, body] : _objects) {
        if (!body->hasCollision() && !body->isCollider() && !body->isTrigger()) continue;

        AABB aabb = body->getAABB();
        double gs = Consts::PHYSICS_GRID_SIZE;

        int minX = static_cast<int>(floor(aabb.min.x() / gs));
        int minY = static_cast<int>(floor(aabb.min.y() / gs));
        int minZ = static_cast<int>(floor(aabb.min.z() / gs));
        int maxX = static_cast<int>(floor(aabb.max.x() / gs));
        int maxY = static_cast<int>(floor(aabb.max.y() / gs));
        int maxZ = static_cast<int>(floor(aabb.max.z() / gs));

        for (int gx = minX; gx <= maxX; gx++) {
            for (int gy = minY; gy <= maxY; gy++) {
                for (int gz = minZ; gz <= maxZ; gz++) {
                    _grid[{gx, gy, gz}].push_back(name);
                }
            }
        }
    }
}

void World::_checkCollisionPair(const ObjectNameTag &a, const ObjectNameTag &b) {
    auto bodyA = _objects[a];
    auto bodyB = _objects[b];

    if (!bodyA->hasCollision()) return;
    if (!bodyB->hasCollision() && !(bodyB->isCollider() || bodyB->isTrigger())) return;

    std::pair<bool, Simplex> gjk = bodyA->checkGJKCollision(bodyB);
    if (gjk.first) {
        if (bodyB->isCollider()) {
            CollisionPoint epa = bodyA->EPA(gjk.second, bodyB);
            bodyA->solveCollision(epa, bodyB);

            // Update contact cache for warm starting
            bool found = false;
            for (auto &c : _contacts) {
                if ((c.a == a && c.b == b) || (c.a == b && c.b == a)) {
                    c.lastContactTime = _physicsTime;
                    found = true;
                    break;
                }
            }
            if (!found) {
                _contacts.push_back({a, b, _physicsTime, Vec3D{0, 0, 0}});
            }
        }
        if (bodyA->collisionCallBack() != nullptr) {
            bodyA->collisionCallBack()(b, bodyB);
        }
    }
}

void World::checkCollision(const ObjectNameTag &tag) {
    // Only used as fallback without broadphase
    if (_objects.find(tag) == _objects.end()) return;
    if (!_objects[tag]->hasCollision()) return;

    _objects[tag]->setInCollision(false);

    for (auto it = _objects.begin(); it != _objects.end();) {
        auto obj = it->second;
        ObjectNameTag name = it->first;
        it++;

        if ((name == tag) || !(obj->isCollider() || obj->isTrigger())) {
            continue;
        }

        std::pair<bool, Simplex> gjk = _objects[tag]->checkGJKCollision(obj);
        if (gjk.first) {
            if (obj->isCollider()) {
                CollisionPoint epa = _objects[tag]->EPA(gjk.second, obj);
                _objects[tag]->solveCollision(epa, obj);
            }
            if (_objects[tag]->collisionCallBack() != nullptr) {
                _objects[tag]->collisionCallBack()(name, obj);
            }
        }
    }
}

void World::stepPhysics(double dt) {
    _physicsTime += dt;

    // Reset collision state for all objects
    for (auto &[nameTag, obj] : _objects) {
        obj->setInCollision(false);
    }

    // Update physics state for all objects (semi-implicit Euler)
    for (auto &[nameTag, obj] : _objects) {
        obj->updatePhysicsState(dt);
    }

    // Broadphase: build spatial grid
    _buildBroadphase();

    // Narrowphase: for each active body, check collisions against grid neighbors
    unordered_set<size_t> visitedPairs;
    for (auto &[nameTag, body] : _objects) {
        if (!body->hasCollision()) continue;

        AABB aabb = body->getAABB();
        double gs = Consts::PHYSICS_GRID_SIZE;
        int minX = static_cast<int>(floor(aabb.min.x() / gs));
        int minY = static_cast<int>(floor(aabb.min.y() / gs));
        int minZ = static_cast<int>(floor(aabb.min.z() / gs));
        int maxX = static_cast<int>(floor(aabb.max.x() / gs));
        int maxY = static_cast<int>(floor(aabb.max.y() / gs));
        int maxZ = static_cast<int>(floor(aabb.max.z() / gs));

        for (int gx = minX; gx <= maxX; gx++) {
            for (int gy = minY; gy <= maxY; gy++) {
                for (int gz = minZ; gz <= maxZ; gz++) {
                    auto cellIt = _grid.find({gx, gy, gz});
                    if (cellIt == _grid.end()) continue;

                    for (auto &otherName : cellIt->second) {
                        if (otherName == nameTag) continue;

                        size_t hashA = std::hash<string>{}(nameTag.str());
                        size_t hashB = std::hash<string>{}(otherName.str());
                        size_t pairHash = hashA ^ (hashB << 1);
                        if (visitedPairs.insert(pairHash).second) {
                            _checkCollisionPair(nameTag, otherName);
                        }
                    }
                }
            }
        }
    }

    // Cleanup stale contacts (older than 1 second)
    _contacts.erase(std::remove_if(_contacts.begin(), _contacts.end(),
        [this](const ContactPair &c) { return _physicsTime - c.lastContactTime > 1.0; }),
        _contacts.end());
}

void World::update() {
    stepPhysics(Time::deltaTime());
}

std::shared_ptr<RigidBody> World::body(const ObjectNameTag &tag) {
    if (_objects.count(tag) == 0) {
        return nullptr;
    }
    return _objects.find(tag)->second;
}
