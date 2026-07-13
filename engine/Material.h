#ifndef ENGINE_MATERIAL_H
#define ENGINE_MATERIAL_H

#include <memory>
#include <string>

#include <SFML/Graphics.hpp>

struct Material {
    std::string name;
    sf::Color color{255, 255, 255};
    std::shared_ptr<sf::Texture> texture = nullptr;
    bool hasTexture = false;
    float alpha = 1.0f;
};

#endif
