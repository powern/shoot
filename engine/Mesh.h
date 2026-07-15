#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <utility>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Triangle.h"
#include "Object.h"
#include "Material.h"

class Mesh : public Object {
private:
    std::vector<Triangle> _tris;
    std::vector<Material> _materials;
    sf::Color _color = sf::Color(255, 245, 194);
    bool _visible = true;
    double _boundingRadius = 0;

    void computeBoundingRadius();

    Mesh &operator*=(const Matrix4x4 &matrix4X4);

    // OpenGL
    mutable GLfloat* _geometry = nullptr;
    mutable GLfloat* _texGeometry = nullptr;
public:
    explicit Mesh(ObjectNameTag nameTag) : Object(std::move(nameTag)) {};

    void scale(const Vec3D &s) {
        Object::scale(s);
        computeBoundingRadius();
    }

    Mesh &operator=(const Mesh &mesh) = delete;

    Mesh(const Mesh &mesh) : Object(mesh),
                             _tris(mesh._tris),
                             _materials(mesh._materials),
                             _color(mesh._color),
                             _visible(mesh._visible),
                             _boundingRadius(mesh._boundingRadius) {};

    explicit Mesh(ObjectNameTag nameTag, const std::vector<Triangle> &tries);

    explicit Mesh(ObjectNameTag nameTag, const std::string &filename, const Vec3D &scale = Vec3D{1, 1, 1});

    void loadObj(const std::string &filename, const Vec3D &scale = Vec3D{1, 1, 1});

    [[nodiscard]] std::vector<Triangle> const &triangles() const { return _tris; }

    [[nodiscard]] std::vector<Material>& materials() { return _materials; }

    void setTriangles(std::vector<Triangle>&& t);

    [[nodiscard]] size_t size() const { return _tris.size() * 3; }

    [[nodiscard]] double boundingRadius() const { return _boundingRadius; }

    [[nodiscard]] sf::Color color() const { return _color; }

    void setColor(const sf::Color &c);

    void setOpacity(double t);

    void setVisible(bool visibility) { _visible = visibility; }

    [[nodiscard]] bool isVisible() const { return _visible; }

    [[nodiscard]] bool hasTextures() const;

    ~Mesh() override;

    Mesh static Cube(ObjectNameTag tag, double size = 1.0, sf::Color color = sf::Color(0,0,0));

    Mesh static LineTo(ObjectNameTag nameTag, const Vec3D &from, const Vec3D &to, double line_width = 0.1,
                       const sf::Color &color = {150, 150, 150, 100});

    Mesh static ArrowTo(ObjectNameTag nameTag, const Vec3D& from, const Vec3D& to, double line_width = 0.1, sf::Color color = {150, 150, 150, 255});

    // OpenGL functions
    GLfloat *glFloatArray() const;
    GLfloat *glTexFloatArray() const;
    void glFreeFloatArray();
};

#endif //INC_3DZAVR_MESH_H