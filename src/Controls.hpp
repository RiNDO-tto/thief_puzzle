#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <array>
#include <string>

enum class Action : int { Up, Left, Down, Right, Undo, Restart, Back, Next, Count };
struct Controls {
    std::array<sf::Keyboard::Key, 8> keys = {sf::Keyboard::Key::W, sf::Keyboard::Key::A,
        sf::Keyboard::Key::S, sf::Keyboard::Key::D, sf::Keyboard::Key::Z,
        sf::Keyboard::Key::R, sf::Keyboard::Key::Q, sf::Keyboard::Key::N};
    sf::Keyboard::Key get(Action a) const { return keys[static_cast<int>(a)]; }
    bool matches(Action a, sf::Keyboard::Key key) const { return get(a) == key; }
    bool assign(Action a, sf::Keyboard::Key key) {
        if (key == sf::Keyboard::Key::Unknown || key == sf::Keyboard::Key::Escape ||
            key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space ||
            (key >= sf::Keyboard::Key::F1 && key <= sf::Keyboard::Key::F15)) return false;
        for (int i=0;i<8;++i) if (i != static_cast<int>(a) && keys[i] == key) return false;
        keys[static_cast<int>(a)] = key;
        return true;
    }
    static std::string name(sf::Keyboard::Key key) {
        using K = sf::Keyboard::Key;
        if (key >= K::A && key <= K::Z) return std::string(1, char('A' + (int(key) - int(K::A))));
        if (key >= K::Num0 && key <= K::Num9) return std::string(1, char('0' + (int(key) - int(K::Num0))));
        if (key >= K::Numpad0 && key <= K::Numpad9) return "Num" + std::to_string(int(key) - int(K::Numpad0));
        switch (key) {
            case K::Up: return "↑"; case K::Down: return "↓";
            case K::Left: return "←"; case K::Right: return "→";
            case K::LShift: return "LShift"; case K::RShift: return "RShift";
            case K::LControl: return "LCtrl"; case K::RControl: return "RCtrl";
            case K::Tab: return "Tab"; case K::Backspace: return "Backspace";
            case K::Hyphen: return "-"; case K::Equal: return "=";
            case K::Comma: return ","; case K::Period: return ".";
            case K::Slash: return "/"; case K::Semicolon: return ";";
            default: return "Key " + std::to_string(int(key));
        }
    }
    std::string label(Action a) const { return name(get(a)); }
    std::string movement() const {
        return label(Action::Up) + "/" + label(Action::Left) + "/" +
               label(Action::Down) + "/" + label(Action::Right);
    }
};
