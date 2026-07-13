#include <map>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>

#include "ResourceManager.h"
#include "Log.h"
#include "../math/Vec2D.h"

ResourceManager *ResourceManager::_instance = nullptr;

void ResourceManager::init() {
    delete _instance;
    _instance = new ResourceManager();

    Log::log("ResourceManager::init(): resource manager was initialized");
}

std::shared_ptr<sf::Texture> ResourceManager::loadTexture(const std::string &filename) {
    if (_instance == nullptr) {
        return nullptr;
    }

    auto it = _instance->_textures.find(filename);
    if (it != _instance->_textures.end()) {
        return it->second;
    }

    std::shared_ptr<sf::Texture> texture(new sf::Texture);
    if (!texture->loadFromFile(filename)) {
        Log::log("ResourceManager::loadTexture: error with loading texture '" + filename + "'");
        return nullptr;
    }

    Log::log("ResourceManager::loadTexture: texture '" + filename + "' was loaded");

    texture->setRepeated(true);
    _instance->_textures.emplace(filename, texture);

    return texture;
}

std::shared_ptr<sf::SoundBuffer> ResourceManager::loadSoundBuffer(const std::string &filename) {
    if (_instance == nullptr) {
        return nullptr;
    }

    auto it = _instance->_soundBuffers.find(filename);
    if (it != _instance->_soundBuffers.end()) {
        return it->second;
    }

    std::shared_ptr<sf::SoundBuffer> soundBuffer(new sf::SoundBuffer);
    if (!soundBuffer->loadFromFile(filename)) {
        Log::log("ResourceManager::loadSoundBuffer: error with loading sound buffer '" + filename + "'");
        return nullptr;
    }

    Log::log("ResourceManager::loadSoundBuffer: sound buffer '" + filename + "' was loaded");

    _instance->_soundBuffers.emplace(filename, soundBuffer);

    return soundBuffer;
}

std::shared_ptr<sf::Font> ResourceManager::loadFont(const std::string &filename) {
    if (_instance == nullptr) {
        return nullptr;
    }

    auto it = _instance->_fonts.find(filename);
    if (it != _instance->_fonts.end()) {
        return it->second;
    }

    std::shared_ptr<sf::Font> font(new sf::Font);
    if (!font->loadFromFile(filename)) {
        Log::log("ResourceManager::loadFont: error with loading font '" + filename + "'");
        return nullptr;
    }

    Log::log("ResourceManager::loadFont: font '" + filename + "' was loaded");

    _instance->_fonts.emplace(filename, font);

    return font;
}

std::vector<Material> ResourceManager::loadMTL(const std::string &filename, const std::string &basePath) {
    std::vector<Material> materials;

    std::ifstream file(filename);
    if (!file.is_open()) {
        Log::log("ResourceManager::loadMTL: cannot open MTL file '" + filename + "'");
        return materials;
    }

    Material current;
    int lineNum = 0;
    std::string line;

    while (std::getline(file, line)) {
        lineNum++;
        std::stringstream s(line);
        std::string cmd;
        s >> cmd;

        if (cmd == "newmtl") {
            if (!current.name.empty()) {
                materials.push_back(current);
            }
            current = Material();
            s >> current.name;
            while (!current.name.empty() && (current.name.back() == '\r')) {
                current.name.pop_back();
            }
        } else if (cmd == "map_Kd") {
            std::string texPath;
            s >> texPath;
            std::string fullPath = basePath + "/" + texPath;
            current.texture = loadTexture(fullPath);
            current.hasTexture = (current.texture != nullptr);
        } else if (cmd == "Kd") {
            float r, g, b;
            s >> r >> g >> b;
            current.color = sf::Color(
                static_cast<sf::Uint8>(r * 255),
                static_cast<sf::Uint8>(g * 255),
                static_cast<sf::Uint8>(b * 255)
            );
        } else if (cmd == "d" || cmd == "Tr") {
            float a;
            s >> a;
            current.alpha = a;
        }
    }

    if (!current.name.empty()) {
        materials.push_back(current);
    }

    Log::log("ResourceManager::loadMTL(): loaded " + std::to_string(materials.size()) + " materials from '" + filename + "'");
    return materials;
}

std::vector<std::shared_ptr<Mesh>> ResourceManager::loadObjects(const std::string &filename) {
    std::vector<std::shared_ptr<Mesh>> objects{};
    std::map<std::string, sf::Color> maters{};
    if (_instance == nullptr) { return objects; }

    auto it = _instance->_objects.find(filename);
    if (it != _instance->_objects.end()) { return it->second; }

    std::ifstream file(filename);
    if (!file.is_open()) {
        Log::log("ResourceManager::loadObjects(): cannot load file from '" + filename + "'");
        return objects;
    }

    std::vector<Vec4D> verts;
    std::vector<Vec2D> uvs;
    std::vector<Vec3D> norms;
    std::vector<Triangle> tris;
    std::vector<Material> materials;
    std::string currentMtlName;
    int currentMtlIndex = -1;
    std::string basePath = std::filesystem::path(filename).parent_path().string();

    // Load MTL if referenced
    bool mtlLoaded = false;

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        std::stringstream s(line);
        char first = 0;
        s >> first;

        if (first == '#' || first == 0) {
            continue;
        }

        if (first == 'm' && line.size() > 1 && line[1] == ' ') {
            // Custom format: m ID R G B A
            std::string matName;
            int color[4];
            s >> matName >> color[0] >> color[1] >> color[2] >> color[3];
            maters.insert({matName, sf::Color(color[0], color[1], color[2], color[3])});
            continue;
        }

        if (line.substr(0, 7) == "mtllib ") {
            if (!mtlLoaded) {
                std::string mtlName = line.substr(7);
                // Remove trailing whitespace
                while (!mtlName.empty() && (mtlName.back() == ' ' || mtlName.back() == '\r')) {
                    mtlName.pop_back();
                }
                std::string mtlPath = basePath.empty() ? mtlName : basePath + "/" + mtlName;
                materials = loadMTL(mtlPath, basePath);
                mtlLoaded = true;
            }
            continue;
        }

        if (first == 'v') {
            if (line.size() > 1 && line[1] == 't') {
                // Texture coordinate: vt u v
                double u, v;
                s >> u >> v;
                uvs.emplace_back(u, v);
            } else if (line.size() > 1 && line[1] == 'n') {
                // Normal: vn x y z
                double x, y, z;
                s >> x >> y >> z;
                norms.emplace_back(x, y, z);
            } else {
                // Vertex position: v x y z
                double x, y, z;
                s >> x >> y >> z;
                verts.emplace_back(x, y, z, 1.0);
            }
            continue;
        }

        if (first == 'u') {
            std::string cmd;
            s >> cmd;
            if (cmd == "usemtl") {
                s >> currentMtlName;
                while (!currentMtlName.empty() && (currentMtlName.back() == '\r')) {
                    currentMtlName.pop_back();
                }
                // Find material index
                currentMtlIndex = -1;
                for (size_t mi = 0; mi < materials.size(); mi++) {
                    if (materials[mi].name == currentMtlName) {
                        currentMtlIndex = static_cast<int>(mi);
                        break;
                    }
                }
            }
            continue;
        }

        if (first == 'f') {
            // Face: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
            std::vector<int> vIdx, vtIdx, vnIdx;
            std::string part;

            while (s >> part) {
                std::stringstream ps(part);
                std::string token;
                int idx[3] = {0, 0, 0};
                int ci = 0;

                while (std::getline(ps, token, '/') && ci < 3) {
                    if (!token.empty()) {
                        idx[ci] = std::stoi(token);
                    }
                    ci++;
                }

                // Convert to 0-based
                if (idx[0] > 0) idx[0]--;
                else if (idx[0] < 0) idx[0] = static_cast<int>(verts.size()) + idx[0];

                if (idx[1] > 0) idx[1]--;
                else if (idx[1] < 0) idx[1] = static_cast<int>(uvs.size()) + idx[1];

                if (idx[2] > 0) idx[2]--;
                else if (idx[2] < 0) idx[2] = static_cast<int>(norms.size()) + idx[2];

                vIdx.push_back(idx[0]);
                vtIdx.push_back(idx[1]);
                vnIdx.push_back(idx[2]);
            }

            // Triangulate: for convex polygons, use triangle fan
            size_t n = vIdx.size();
            if (n >= 3) {
                sf::Color faceColor(255, 255, 255);
                if (currentMtlIndex < 0 || currentMtlIndex >= (int)materials.size()) {
                    // Try custom material from 'g' line (backward compat)
                } else {
                    faceColor = materials[currentMtlIndex].color;
                }

                for (size_t fi = 1; fi < n - 1; fi++) {
                    Vec4D p0 = (vIdx[0] < (int)verts.size()) ? verts[vIdx[0]] : Vec4D(0,0,0,1);
                    Vec4D p1 = (vIdx[fi] < (int)verts.size()) ? verts[vIdx[fi]] : Vec4D(0,0,0,1);
                    Vec4D p2 = (vIdx[fi+1] < (int)verts.size()) ? verts[vIdx[fi+1]] : Vec4D(0,0,0,1);

                    Vec2D uv0, uv1, uv2;
                    if (vtIdx[0] < (int)uvs.size()) uv0 = uvs[vtIdx[0]];
                    if (vtIdx[fi] < (int)uvs.size()) uv1 = uvs[vtIdx[fi]];
                    if (vtIdx[fi+1] < (int)uvs.size()) uv2 = uvs[vtIdx[fi+1]];

                    if (currentMtlIndex >= 0 && currentMtlIndex < (int)materials.size()) {
                        tris.emplace_back(p0, p1, p2, uv0, uv1, uv2, currentMtlIndex, faceColor);
                    } else {
                        tris.emplace_back(p0, p1, p2, faceColor);
                    }
                }
            }
            continue;
        }

        if (first == 'o') {
            // Object separator
            if (!tris.empty()) {
                auto mesh = std::make_shared<Mesh>(
                    ObjectNameTag(filename + "_temp_obj_" + std::to_string(objects.size())), tris);
                mesh->materials() = materials;
                objects.push_back(mesh);
                tris.clear();
            }
            continue;
        }

        if (first == 'g') {
            // Group - handle old format material assignment
            std::string matInfo;
            s >> matInfo;
            if (matInfo.size() >= 3) {
                std::string colorKey = matInfo.substr(matInfo.size() - 3, 3);
                auto matIt = maters.find(colorKey);
                if (matIt != maters.end()) {
                    // Old format: try to find or create a material
                    bool found = false;
                    for (auto &m : materials) {
                        if (m.name == colorKey) {
                            currentMtlIndex = static_cast<int>(&m - &materials[0]);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        Material mat;
                        mat.name = colorKey;
                        mat.color = matIt->second;
                        currentMtlIndex = static_cast<int>(materials.size());
                        materials.push_back(mat);
                    }
                }
            }
            continue;
        }

        if (first == 's') {
            // Smoothing group - ignore
            continue;
        }
    }

    if (!tris.empty()) {
        auto mesh = std::make_shared<Mesh>(
            ObjectNameTag(filename + "_temp_obj_" + std::to_string(objects.size())), tris);
        mesh->materials() = materials;
        objects.push_back(mesh);
    }

    file.close();
    Log::log("ResourceManager::loadObjects(): obj '" + filename + "' was loaded (" +
             std::to_string(verts.size()) + " verts, " +
             std::to_string(uvs.size()) + " uvs, " +
             std::to_string(norms.size()) + " norms, " +
             std::to_string(tris.size()) + " tris, " +
             std::to_string(materials.size()) + " mats)");
    _instance->_objects.emplace(filename, objects);
    return objects;
}

void ResourceManager::unloadObjects() {
    if (_instance != nullptr) {
        _instance->_objects.clear();
    }
}

void ResourceManager::unloadTextures() {
    if (_instance != nullptr) {
        _instance->_textures.clear();
    }
}

void ResourceManager::unloadSoundBuffers() {
    if (_instance != nullptr) {
        _instance->_soundBuffers.clear();
    }
}

void ResourceManager::unloadFonts() {
    if (_instance != nullptr) {
        _instance->_fonts.clear();
    }
}

void ResourceManager::unloadAllResources() {
    unloadObjects();
    unloadTextures();
    unloadSoundBuffers();
    unloadFonts();
    Log::log("ResourceManager::unloadAllResources(): all resources were unloaded");
}

void ResourceManager::free() {
    delete _instance;
    _instance = nullptr;
}
