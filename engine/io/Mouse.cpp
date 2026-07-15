#include "Mouse.h"
#include "../utils/Time.h"
#include "../Consts.h"

Vec2D Mouse::getMousePosition() const {
    sf::Vector2<int> pos = sf::Mouse::getPosition(*_screen->renderWindow());
    return Vec2D(pos.x, pos.y);
}

Vec2D Mouse::getMouseDisplacement() const {
    sf::Vector2i delta = _screen->getAndResetMouseDelta();
    return Vec2D(static_cast<double>(delta.x), static_cast<double>(delta.y));
}

bool Mouse::isButtonPressed(sf::Mouse::Button button) {
    return sf::Mouse::isButtonPressed(button);
}

bool Mouse::isButtonTapped(sf::Mouse::Button button) {
    if (!Mouse::isButtonPressed(button)) {
        return false;
    }

    if (_tappedButtons.count(button) == 0) {
        _tappedButtons.emplace(button, Time::time());
        return true;
    } else if ((Time::time() - _tappedButtons[button]) > Consts::TAP_DELAY) {
        _tappedButtons[button] = Time::time();
        return true;
    }
    return false;
}
