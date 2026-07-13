#ifndef ENGINE_TRIANGLE_H
#define ENGINE_TRIANGLE_H

#include <SFML/Graphics.hpp>

#include "math/Vec2D.h"
#include "math/Vec4D.h"
#include "math/Vec3D.h"
#include "math/Matrix4x4.h"

class Triangle final {
private:
    sf::Color _color;
    Vec4D _points[3];
    Vec2D _uv[3]{};
    Vec3D _normal;
    int _materialIndex = -1;

    void calculateNormal();
public:
    Triangle() = default;

    Triangle(const Triangle &triangle) = default;

    Triangle &operator=(const Triangle &) = default;

    Triangle(const Vec4D &p1, const Vec4D &p2, const Vec4D &p3, sf::Color color = {0, 0, 0});

    Triangle(const Vec4D &p1, const Vec4D &p2, const Vec4D &p3,
             const Vec2D &uv1, const Vec2D &uv2, const Vec2D &uv3,
             int materialIndex, sf::Color color = {255, 255, 255});

    [[nodiscard]] const Vec4D& operator[](int i) const;

    [[nodiscard]] const Vec2D& uv(int i) const { return _uv[i]; }

    [[nodiscard]] Vec3D position() const { return Vec3D(_points[0] + _points[1] + _points[2])/3; }

    [[nodiscard]] Vec3D norm() const;

    [[nodiscard]] Triangle operator*(const Matrix4x4 &matrix4X4) const;

    [[nodiscard]] bool isPointInside(const Vec3D &point) const;

    [[nodiscard]] sf::Color color() const { return _color; }

    void setColor(sf::Color newColor) { _color = newColor; }

    [[nodiscard]] int materialIndex() const { return _materialIndex; }

    [[nodiscard]] double distance(const Vec3D &vec) const { return norm().dot(Vec3D(_points[0]) - vec); }
};


#endif //INC_3DZAVR_TRIANGLE_H
