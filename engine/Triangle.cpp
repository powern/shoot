#include "Triangle.h"
#include "Consts.h"

Triangle::Triangle(const Vec4D &p1, const Vec4D &p2, const Vec4D &p3, sf::Color color) : _color(color),
                                                                                          _points{p1, p2, p3} {
    calculateNormal();
}

Triangle::Triangle(const Vec4D &p1, const Vec4D &p2, const Vec4D &p3,
                   const Vec2D &uv1, const Vec2D &uv2, const Vec2D &uv3,
                   int materialIndex, sf::Color color)
    : _color(color), _points{p1, p2, p3}, _uv{uv1, uv2, uv3}, _materialIndex(materialIndex) {
    calculateNormal();
}

void Triangle::calculateNormal() {
    Vec3D v1 = Vec3D(_points[1] - _points[0]);
    Vec3D v2 = Vec3D(_points[2] - _points[0]);
    Vec3D crossProduct = v1.cross(v2);

    if (crossProduct.sqrAbs() > Consts::EPS) {
        _normal = crossProduct.normalized();
    } else {
        _normal = Vec3D(0);
    }
}

Triangle Triangle::operator*(const Matrix4x4 &matrix4X4) const {
    Triangle result(matrix4X4 * _points[0], matrix4X4 * _points[1], matrix4X4 * _points[2], _color);
    result._uv[0] = _uv[0];
    result._uv[1] = _uv[1];
    result._uv[2] = _uv[2];
    result._materialIndex = _materialIndex;
    return result;
}

Vec3D Triangle::norm() const {
    return _normal;
}

const Vec4D& Triangle::operator[](int i) const {
    return _points[i];
}

bool Triangle::isPointInside(const Vec3D &point) const {
    Vec3D triangleNorm = norm();

    double dot1 = (point - Vec3D(_points[0])).cross(Vec3D(_points[1] - _points[0])).dot(triangleNorm);
    double dot2 = (point - Vec3D(_points[1])).cross(Vec3D(_points[2] - _points[1])).dot(triangleNorm);
    double dot3 = (point - Vec3D(_points[2])).cross(Vec3D(_points[0] - _points[2])).dot(triangleNorm);

    if ((dot1 >= 0 && dot2 >= 0 && dot3 >= 0) || (dot1 <= 0 && dot2 <= 0 && dot3 <= 0)) {
        return true;
    }
    return false;
}
