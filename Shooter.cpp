#include "Shooter.h"
#include <fstream>
#include <utility>
#include <algorithm>
#include "engine/animation/Animations.h"
#include "ShooterConsts.h"
#include "engine/utils/Log.h"
#include "engine/io/SoundController.h"

struct MapConfig {
    std::string path;
    Vec3D scale;
    Matrix4x4 transform;
    Vec3D playerSpawn;
    bool useTextures;
};

static const MapConfig LEGACY_MAP_CONFIG{
    ShooterConsts::MAP_OBJ,
    Vec3D{5, 5, 5},
    Matrix4x4::Identity(),
    Vec3D{0, 10, 0},
    false
};

static const MapConfig DOOM_MAP_CONFIG{
    ShooterConsts::DOOM_MAP_OBJ,
    Vec3D{0.1, 0.1, 0.1},
    Matrix4x4::RotationX(-Consts::PI / 2.0),
    Vec3D{0, 1.0, 0},
    false
};

using namespace std;

// Read server/client settings and start both.
// If client doesn't connect to the localhost - server doesn't start.
void Shooter::initNetwork() {
    std::string clientIp;
    sf::Uint16 clientPort;
    sf::Uint16 serverPort;
    std::string playerName;
    std::ifstream connectFile("connect.txt", std::ifstream::in);

    // If failed to read client settings
    if (!connectFile.is_open() || !(connectFile >> clientIp >> clientPort >> playerName) ||
        sf::IpAddress(clientIp) == sf::IpAddress::None) {
        connectFile.close();
        // Create file and write default settings
        clientIp = "127.0.0.1";
        clientPort = 54000;
        playerName = "PlayerName";
        std::ofstream temp("connect.txt", std::ofstream::out);
        temp << clientIp << std::endl << clientPort << std::endl << playerName;
        temp.close();
    }
    connectFile.close();

    // If failed to read server settings
    connectFile.open("server.txt", std::ifstream::in);
    if (!connectFile.is_open() || !(connectFile >> serverPort)) {
        connectFile.close();
        // Create file and write default settings
        serverPort = 54000;
        std::ofstream temp("server.txt", std::ofstream::out);
        temp << serverPort;
        temp.close();
    }
    connectFile.close();

    if (clientIp == sf::IpAddress::LocalHost) {
        server->start(serverPort);
        if (server->isWorking())
            server->generateBonuses();
    }

    client->connect(clientIp, clientPort);
    player->setPlayerNickName(playerName);

    // TODO: encapsulate call backs inside ShooterClient
    client->setSpawnPlayerCallBack([this](sf::Uint16 id) { spawnPlayer(id); });
    client->setRemovePlayerCallBack([this](sf::Uint16 id) { removePlayer(id); });
    client->setAddFireTraceCallBack([this](const Vec3D &from, const Vec3D &to) { addFireTrace(from, to); });
    client->setAddBonusCallBack(
            [this](const std::string &bonusName, const Vec3D &position) { addBonus(bonusName, position); });
    client->setRemoveBonusCallBack([this](const ObjectNameTag &bonusName) { removeBonus(bonusName); });
    client->setChangeEnemyWeaponCallBack(
            [this](const std::string &weaponName, sf::Uint16 id) { changeEnemyWeapon(weaponName, id); });
}

void Shooter::start() {
    // This code executed once in the beginning:
    setUpdateWorld(false);

    screen->setMouseCursorVisible(true);

    const MapConfig &mapConfig = DOOM_MAP_CONFIG;

    world->loadMap(mapConfig.path, mapConfig.scale, mapConfig.transform);

    // When textures are disabled, assign visible fallback colors to geometry without vertex colors
    if (!mapConfig.useTextures) {
        for (auto &it : *world) {
            if (!it.second->materials().empty()) {
                it.second->setColor(sf::Color(160, 160, 160));
            }
        }
    }

    Log::log("=== MAP DIAGNOSTICS ===");
    Log::log("Config: path=" + mapConfig.path + " scale=" +
             std::to_string(mapConfig.scale.x()) + "," +
             std::to_string(mapConfig.scale.y()) + "," +
             std::to_string(mapConfig.scale.z()) + " useTextures=" +
             std::to_string(mapConfig.useTextures));
    Log::log("Spawn: " + std::to_string(mapConfig.playerSpawn.x()) + " " +
             std::to_string(mapConfig.playerSpawn.y()) + " " +
             std::to_string(mapConfig.playerSpawn.z()));

    // TODO: encapsulate call backs inside Player
    player->setAddTraceCallBack([this](const Vec3D &from, const Vec3D &to) {
        client->addTrace(from, to);
        addFireTrace(from, to);
    });
    player->setDamagePlayerCallBack(
            [this](sf::Uint16 targetId, double damage) { client->damagePlayer(targetId, damage); });
    player->setRayCastFunction(
            [this](const Vec3D &from, const Vec3D &to) { return world->rayCast(from, to, "Player Weapon fireTrace bulletHole"); });
    player->setTakeBonusCallBack([this](const string &bonusName) { client->takeBonus(bonusName); });
    player->setAddWeaponCallBack([this](std::shared_ptr<Weapon> weapon) { addWeapon(std::move(weapon)); });
    player->setRemoveWeaponCallBack([this](std::shared_ptr<Weapon> weapon) { removeWeapon(std::move(weapon)); });

    player->reInitWeapons();

    player->translateToPoint(mapConfig.playerSpawn);
    camera->translateToPoint(player->position() + Vec3D{0, 1.8, 0});
    player->setAcceleration(Vec3D{0, 0, 0});
    player->attach(camera);
    world->addBody(player);

    // connecting to the server
    initNetwork();
    // Waiting for connect and updating server if it's same window
    while (client->isWorking() && !client->connected()) {
        client->update();
        server->update();
        Time::update();
    }
    // If connect fail - return to menu
    if (!client->isWorking()) {
        inGame = false;
        server->stop();
    }

    // windows init:
    mainMenu.setTitle("Main menu");
    mainMenu.setBackgroundTexture(ShooterConsts::MAIN_MENU_BACK, 1.1, 1.1, screen->width(), screen->height());

    mainMenu.addButton(screen->width() / 2, 200, 200, 20, [this]() {
                           this->play();
                           SoundController::loadAndPlay(SoundTag("click"), ShooterConsts::CLICK_SOUND);
                       }, "Server: " + client->serverIp().toString(), 5, 5, ShooterConsts::MAIN_MENU_GUI, {0, 66}, {0, 86}, {0, 46},
                       Consts::MEDIUM_FONT, {255, 255, 255});
    mainMenu.addButton(screen->width() / 2, 350, 200, 20, [this]() {
        this->player->translateToPoint(DOOM_MAP_CONFIG.playerSpawn);
        this->player->setVelocity({});
        this->play();
        SoundController::loadAndPlay(SoundTag("click"), ShooterConsts::CLICK_SOUND);
    }, "Respawn", 5, 5, ShooterConsts::MAIN_MENU_GUI, {0, 66}, {0, 86}, {0, 46}, Consts::MEDIUM_FONT, {255, 255, 255});

    mainMenu.addButton(screen->width() / 2, 500, 200, 20, [this]() {
        client->disconnect();
        server->stop();
        this->exit();
    }, "Exit", 5, 5, ShooterConsts::MAIN_MENU_GUI, {0, 66}, {0, 86}, {0, 46}, Consts::MEDIUM_FONT, {255, 255, 255});

    if (_autoStart) {
        play();
    }
}

void Shooter::update() {
    // This code executed every time step:

    server->update();
    client->update();

    // Check all input after this condition please

    if (keyboard->isKeyTapped(sf::Keyboard::Escape)) {
        inGame = !inGame;
        screen->setMouseCursorVisible(!inGame);
        screen->renderWindow()->setMouseCursorGrabbed(inGame);
    }

    if (keyboard->isKeyTapped(sf::Keyboard::O)) {
        setGlEnable(!glEnable());
    }

    if (keyboard->isKeyTapped(sf::Keyboard::Tab)) {
        setDebugInfo(!showDebugInfo());
    }

    if (keyboard->isKeyTapped(sf::Keyboard::F3)) {
        Log::log("=== DEBUG INFO ===");
        Log::log("Camera pos: " + std::to_string(camera->position().x()) + ", " +
                 std::to_string(camera->position().y()) + ", " + std::to_string(camera->position().z()));
        Log::log("Camera lookAt: " + std::to_string(camera->lookAt().x()) + ", " +
                 std::to_string(camera->lookAt().y()) + ", " + std::to_string(camera->lookAt().z()));
        Log::log("Camera angles (LUL): " + std::to_string(camera->angleLeftUpLookAt().x()) + ", " +
                 std::to_string(camera->angleLeftUpLookAt().y()) + ", " + std::to_string(camera->angleLeftUpLookAt().z()));
        Log::log("FPS: " + std::to_string(Time::fps()));
        Log::log("Tri count: " + std::to_string(camera->buffSize()));
        Log::log("=== END DEBUG ===");
    }

    if (keyboard->isKeyTapped(sf::Keyboard::F4)) {
        Log::log("=== RENDER DEBUG ===");
        Log::log("OpenGL: " + std::string(glEnable() ? "ON" : "OFF"));
        Log::log("Camera pos: " + std::to_string(camera->position().x()) + ", " +
                 std::to_string(camera->position().y()) + ", " + std::to_string(camera->position().z()));
        Log::log("Camera lookAt: " + std::to_string(camera->lookAt().x()) + ", " +
                 std::to_string(camera->lookAt().y()) + ", " + std::to_string(camera->lookAt().z()));
        Log::log("World bodies:");
        for (auto &it : *world) {
            if (it.second->isVisible()) {
                Log::log("  visible: " + it.second->name().str() + " (" +
                         std::to_string(it.second->triangles().size()) + " tris)" +
                         " pos=(" + std::to_string(it.second->position().x()) + "," +
                         std::to_string(it.second->position().y()) + "," +
                         std::to_string(it.second->position().z()) + ")" +
                         " triangles col=" +
                         std::to_string(it.second->triangles().empty() ? 0 : it.second->triangles()[0].color().r) + "," +
                         std::to_string(it.second->triangles().empty() ? 0 : it.second->triangles()[0].color().g) + "," +
                         std::to_string(it.second->triangles().empty() ? 0 : it.second->triangles()[0].color().b));
            }
        }
        Log::log("=== END RENDER DEBUG ===");
    }

    if (keyboard->isKeyTapped(sf::Keyboard::F6)) {
        _debugRotate = true;
        _debugRotateFrame = 0;
        Log::log("=== AUTO-ROTATE ON ===");
    }

    if (keyboard->isKeyTapped(sf::Keyboard::P)) {
        screen->startRender();
    }

    if (keyboard->isKeyTapped(sf::Keyboard::L)) {
        screen->stopRender();
    }

    if (inGame) {
        screen->setTitle(ShooterConsts::PROJECT_NAME);

        if (_debugRotate) {
            _debugRotateFrame++;
            if (_debugRotateFrame * 0.003 > 0.5236) {
                _debugRotate = false;
                Log::log("=== AUTO-ROTATE OFF (reached 30deg, frames=" + std::to_string(_debugRotateFrame) + ") ===");
            } else {
                player->rotate(Vec3D{0, 0.003, 0});
            }
            if (_debugRotateFrame % 3 == 0) {
                size_t worldTotal = 0, worldRamp = 0, worldFloor = 0;
                std::vector<std::string> greenBodies, floorBodies;
                for (auto &it : *world) {
                    if (!it.second->triangles().empty()) {
                        worldTotal += it.second->triangles().size();
                        auto c = it.second->triangles()[0].color();
                        if (c.r == 85 && c.g == 231 && c.b == 139) {
                            worldRamp += it.second->triangles().size();
                            greenBodies.push_back(it.second->name().str());
                        }
                        if (c.r == 144 && c.g == 103 && c.b == 84) {
                            worldFloor += it.second->triangles().size();
                            floorBodies.push_back(it.second->name().str());
                        }
                    }
                }
                int camTris = camera->buffSize();
                int camTotalFromBodies = 0, camRamp = 0, camFloor = 0;
                std::string rampDetail, floorDetail;
                for (auto &[name, count] : camera->bodyTrisCount()) {
                    camTotalFromBodies += count;
                    for (auto &gb : greenBodies) {
                        if (name == gb) {
                            camRamp += count;
                            rampDetail += " " + name + ":" + std::to_string(count);
                            break;
                        }
                    }
                    for (auto &fb : floorBodies) {
                        if (name == fb) {
                            camFloor += count;
                            floorDetail += " " + name + ":" + std::to_string(count);
                            break;
                        }
                    }
                }
                std::vector<std::pair<std::string, size_t>> sortedBodies(camera->bodyTrisCount().begin(), camera->bodyTrisCount().end());
                std::sort(sortedBodies.begin(), sortedBodies.end(), [](auto &a, auto &b) { return a.second > b.second; });
                std::string top5;
                for (int bi = 0; bi < std::min(5, (int)sortedBodies.size()); bi++) {
                    top5 += " " + sortedBodies[bi].first + ":" + std::to_string(sortedBodies[bi].second);
                }
                Log::log("ROTATE " + std::to_string(_debugRotateFrame) +
                         " rot=" + std::to_string(_debugRotateFrame * 0.003) +
                         " world=" + std::to_string(worldTotal) +
                         " worldRamp=" + std::to_string(worldRamp) +
                         " worldFloor=" + std::to_string(worldFloor) +
                         " cam=" + std::to_string(camTris) +
                         " camFromBodies=" + std::to_string(camTotalFromBodies) +
                         " camRamp=" + std::to_string(camRamp) +
                         " camFloor=" + std::to_string(camFloor) +
                         " rampDetail=[" + rampDetail + "]" +
                         " floorDetail=[" + floorDetail + "]" +
                         " top5=[" + top5 + "]");
            }
        } else {
            playerController->update();
        }
    } else {
        mainMenu.update();
    }

    setUpdateWorld(inGame);

    // background sounds and music control
    if (SoundController::getStatus(SoundTag("background")) != sf::Sound::Status::Playing) {
        SoundController::loadAndPlay(SoundTag("background"), ShooterConsts::BACK_NOISE);
    }
}

void Shooter::gui() {
    sf::Sprite sprite;
    sprite.setTexture(*ResourceManager::loadTexture(ShooterConsts::MAIN_MENU_GUI));
    sprite.setTextureRect(sf::IntRect(243, 3, 9, 9));
    sprite.scale(3, 3);
    sprite.setPosition(static_cast<float>(screen->width()) / 2.0f - 27.0f / 2.0f,
                       static_cast<float>(screen->height()) / 2.0f - 27.0f / 2.0f);
    sprite.setColor(sf::Color(0, 0, 0, 250));
    screen->drawSprite(sprite);

    // health player stats
    drawPlayerStats();
    drawStatsTable();
}

void Shooter::drawStatsTable() {

    int i = 1;

    screen->drawText(client->lastEvent(), Vec2D{10, 10}, 25, sf::Color(0, 0, 0, 100));

    vector<shared_ptr < Player>>
    allPlayers;
    allPlayers.push_back(player);
    for (auto&[playerId, player] : client->players())
        allPlayers.push_back(player);

    std::sort(allPlayers.begin(), allPlayers.end(), [](std::shared_ptr<Player> p1, std::shared_ptr<Player> p2) {
        return p1->kills() - p1->deaths() > p2->kills() - p2->deaths();
    });

    for (auto &p : allPlayers) {
        screen->drawText(std::to_string(i) + "\t" + p->playerNickName() + "\t" + std::to_string(p->kills()) + " / " +
                         std::to_string(p->deaths()),
                         Vec2D{10, 15 + 35.0 * i}, 25, sf::Color(0, 0, 0, 150));
        i++;
    }

}

void Shooter::drawPlayerStats() {
    // health bar
    double xPos = 10;
    double yPos = screen->height() - 20;

    int width = screen->width() / 2 - 20;
    int height = 10;

    screen->drawTetragon(Vec2D{xPos, yPos},
                         Vec2D{xPos + width * player->health() / ShooterConsts::HEALTH_MAX, yPos},
                         Vec2D{xPos + width * player->health() / ShooterConsts::HEALTH_MAX, yPos + height},
                         Vec2D{xPos, yPos + height},
                         {static_cast<sf::Uint8>((ShooterConsts::HEALTH_MAX - player->health()) /
                                                 ShooterConsts::HEALTH_MAX * 255),
                          static_cast<sf::Uint8>(player->health() * 255 / ShooterConsts::HEALTH_MAX), 0, 100});

    screen->drawTetragon(Vec2D{xPos, yPos - 15},
                         Vec2D{xPos + width * player->ability() / ShooterConsts::ABILITY_MAX, yPos - 15},
                         Vec2D{xPos + width * player->ability() / ShooterConsts::ABILITY_MAX, yPos - 15 + height},
                         Vec2D{xPos, yPos - 15 + height},
                         {255, 168, 168, 100});

    auto balance = player->weapon()->balance();

    screen->drawText(std::to_string((int) balance.first), Vec2D{150, static_cast<double>(screen->height() - 150)}, 100,
                     sf::Color(0, 0, 0, 100));
    screen->drawText(std::to_string((int) balance.second), Vec2D{50, static_cast<double>(screen->height() - 100)}, 50,
                     sf::Color(0, 0, 0, 70));
}

void Shooter::play() {
    inGame = true;
    screen->setMouseCursorVisible(false);
    screen->renderWindow()->setMouseCursorGrabbed(true);
    sf::Vector2i _ignore = screen->getAndResetMouseDelta(); (void)_ignore;
}

void Shooter::spawnPlayer(sf::Uint16 id) {
    std::string name = "Enemy_" + std::to_string(id);

    std::shared_ptr<Player> newPlayer = std::make_shared<Player>(ObjectNameTag(name), ShooterConsts::BODY_OBJ, Vec3D{0.4, 0.4, 0.4});

    client->addPlayer(id, newPlayer);
    world->addBody(newPlayer);
    newPlayer->setVisible(true);
    newPlayer->setCollision(false);
    newPlayer->setAcceleration(Vec3D{0, 0, 0});

    // add head and other stuff:
    world->loadBody(ObjectNameTag(name + "_head"), ShooterConsts::HEAD_OBJ, Vec3D{0.4, 0.4, 0.4});
    world->body(ObjectNameTag(name + "_head"))->translate(Vec3D{0, 2.2, 0});
    newPlayer->attach(world->body(ObjectNameTag(name + "_head")));

    world->loadBody(ObjectNameTag(name + "_foot_1"), ShooterConsts::FOOT_OBJ, Vec3D{0.4, 0.4, 0.4});
    world->body(ObjectNameTag(name + "_foot_1"))->translate(Vec3D{-0.25, 0, 0});
    newPlayer->attach(world->body(ObjectNameTag(name + "_foot_1")));

    world->loadBody(ObjectNameTag(name + "_foot_2"), ShooterConsts::FOOT_OBJ, Vec3D{0.4, 0.4, 0.4});
    world->body(ObjectNameTag(name + "_foot_2"))->translate(Vec3D{0.25, 0, 0});
    newPlayer->attach(world->body(ObjectNameTag(name + "_foot_2")));

    int colorBodyNum = static_cast<int> (static_cast<double>((rand() - 1)) / RAND_MAX * 5.0);
    int colorFootNum = static_cast<int> (static_cast<double>((rand() - 1)) / RAND_MAX * 5.0);

    newPlayer->setColor(Consts::WHITE_COLORS[colorBodyNum]);
    world->body(ObjectNameTag(name + "_foot_1"))->setColor(Consts::DARK_COLORS[colorFootNum]);
    world->body(ObjectNameTag(name + "_foot_2"))->setColor(Consts::DARK_COLORS[colorFootNum]);

    changeEnemyWeapon("gun", id);
}

void Shooter::removePlayer(sf::Uint16 id) {
    std::string name = "Enemy_" + std::to_string(id);

    auto playerToRemove = world->body(ObjectNameTag(name));

    Timeline::addAnimation<AScale>(AnimationListTag(name + "_remove"), playerToRemove, Vec3D(0.01, 0.01, 0.01));
    Timeline::addAnimation<AFunction>(AnimationListTag(name + "_remove"), [this, name](){
        world->removeBody(ObjectNameTag(name));
        world->removeBody(ObjectNameTag(name + "_head"));
        world->removeBody(ObjectNameTag(name + "_weapon"));
        world->removeBody(ObjectNameTag(name + "_foot_1"));
        world->removeBody(ObjectNameTag(name + "_foot_2"));
    });
}

void Shooter::addFireTrace(const Vec3D &from, const Vec3D &to) {
    std::string traceName = "Client_fireTrace_" + std::to_string(fireTraces++);
    world->addBody(std::make_shared<RigidBody>(Mesh::LineTo(ObjectNameTag(traceName), from, to, 0.05)));
    world->body(ObjectNameTag(traceName))->setCollider(false);

    Timeline::addAnimation<AColor>(AnimationListTag(traceName + "_fadeOut"), world->body(ObjectNameTag(traceName)),
                                   sf::Color{150, 150, 150, 0});
    Timeline::addAnimation<AFunction>(AnimationListTag(traceName + "_delete"),
                                      [this, traceName]() { removeFireTrace(ObjectNameTag(traceName)); }, 1,
                                      1);


    std::string bulletHoleName = "Client_bulletHole_" + std::to_string(fireTraces++);
    auto bulletHole = Mesh::Cube(ObjectNameTag(bulletHoleName), 0.2, sf::Color(70, 70, 70));
    bulletHole.translate(to);
    world->addBody(std::make_shared<RigidBody>(bulletHole));
    world->body(ObjectNameTag(bulletHoleName))->setCollider(false);

    Timeline::addAnimation<AFunction>(AnimationListTag(bulletHoleName + "_delete"),
                                      [this, bulletHoleName]() { removeFireTrace(ObjectNameTag(bulletHoleName)); }, 1,
                                      7);
}

void Shooter::removeFireTrace(const ObjectNameTag &traceName) {
    world->removeBody(traceName);
}

void Shooter::addBonus(const string &bonusName, const Vec3D &position) {

    std::string name = bonusName.substr(6, bonusName.size() - 3 - 5);

    ObjectNameTag nameTag(bonusName);

    world->addBody(std::make_shared<RigidBody>(ObjectNameTag(bonusName), "obj/items/" + name + ".obj", Vec3D{3, 3, 3}));

    auto bonus = world->body(ObjectNameTag(bonusName));

    bonus->translateToPoint(position);
    bonus->setCollider(false);
    bonus->setTrigger(true);
    bonus->scale(Vec3D(0.01, 0.01, 0.01));
    Timeline::addAnimation<AScale>(AnimationListTag(bonusName + "_creation"), bonus, Vec3D(100, 100, 100));
    Timeline::addAnimation<ARotate>(AnimationListTag(bonusName + "_rotation"),
                                    bonus, Vec3D{0, 2 * Consts::PI, 0}, 4,
                                    Animation::LoopOut::Continue,
                                    Animation::InterpolationType::Linear);

}

void Shooter::removeBonus(const ObjectNameTag &bonusName) {
    world->removeBody(bonusName);
    Timeline::deleteAnimationList(AnimationListTag(bonusName.str() + "_rotation"));
}

void Shooter::addWeapon(std::shared_ptr<Weapon> weapon) {
    world->addBody(weapon);

    if (client != nullptr) {
        client->changeWeapon(weapon->name().str());
    }
}

void Shooter::changeEnemyWeapon(const std::string &weaponName, sf::Uint16 enemyId) {
    ObjectNameTag weaponTag("Enemy_" + std::to_string(enemyId) + "_weapon");
    auto head = world->body(ObjectNameTag("Enemy_" + std::to_string(enemyId) + "_head"));
    auto enemy = world->body(ObjectNameTag("Enemy_" + std::to_string(enemyId)));
    auto weapon = world->body(weaponTag);

    // remove old weapon:
    world->removeBody(weaponTag);
    enemy->unattach(weaponTag);

    world->loadBody(weaponTag, "obj/items/" + weaponName + ".obj");
    world->body(weaponTag)->setCollider(false);
    world->body(weaponTag)->scale(Vec3D(3, 3, 3));

    world->body(weaponTag)->translateToPoint(head->position() - enemy->left() * 1.0 - enemy->up() * 1.0 + enemy->lookAt());

    world->body(weaponTag)->rotate(Vec3D(0, enemy->angle().y(), 0));
    world->body(weaponTag)->rotateLeft(head->angleLeftUpLookAt().x());
    enemy->attach(world->body(weaponTag));

    Timeline::addAnimation<ARotateLeft>(AnimationListTag("select_weapon_" + std::to_string(enemyId)),
                                        world->body(weaponTag),
                                        -2 * Consts::PI,
                                        0.3,
                                        Animation::LoopOut::None,
                                        Animation::InterpolationType::Cos);
}

void Shooter::removeWeapon(std::shared_ptr<Weapon> weapon) {
    world->removeBody(weapon->name());
}
