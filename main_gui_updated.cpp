// SFML 3 frontend for the existing console simulator.
// Keep main2-2.cpp in the same directory. Its game rules are reused unchanged.
#define main console_simulator_main
#include "main2-2.cpp"
#undef main

#include <SFML/Graphics.hpp>
#include <optional>
#include <algorithm>
#include <cmath>

namespace {
constexpr float TILE = 76.f;
constexpr float MARGIN = 30.f;
constexpr float WALL_THICKNESS = 8.f;

void rect(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size,
          sf::Color color) {
    sf::RectangleShape shape(size);
    shape.setPosition(pos);
    shape.setFillColor(color);
    window.draw(shape);
}

void drawCharacter(sf::RenderWindow& window, const Character& c,
                   sf::Color color, bool controlled) {
    const sf::Vector2f center(MARGIN + (c.x + .5f) * TILE,
                              MARGIN + (c.y + .5f) * TILE);
    if (controlled) {
        sf::CircleShape ring(27.f);
        ring.setOrigin({27.f, 27.f});
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(255, 211, 65));
        ring.setOutlineThickness(4.f);
        window.draw(ring);
    }
    sf::CircleShape body(20.f);
    body.setOrigin({20.f, 20.f});
    body.setPosition(center);
    body.setFillColor(color);
    window.draw(body);

    // White triangle pointing in the character's current direction.
    sf::ConvexShape arrow(3);
    arrow.setPoint(0, {0.f, -15.f});
    arrow.setPoint(1, {-9.f, 8.f});
    arrow.setPoint(2, {9.f, 8.f});
    arrow.setOrigin({0.f, 0.f});
    arrow.setPosition(center);
    arrow.setRotation(sf::degrees(90.f * c.direction));
    arrow.setFillColor(sf::Color::White);
    window.draw(arrow);
}

void drawBoard(sf::RenderWindow& window, const Board& b, const State& s) {
    for (int y = 0; y < b.height; ++y) {
        for (int x = 0; x < b.width; ++x) {
            sf::Vector2f pos(MARGIN + x * TILE, MARGIN + y * TILE);
            rect(window, pos, {TILE - 1.f, TILE - 1.f}, sf::Color(56, 61, 75));
            char tile = b.cell[y][x];
            if (tile == 'A') {
                rect(window, pos + sf::Vector2f(9.f, 9.f),
                     {TILE - 18.f, TILE - 18.f}, sf::Color(52, 157, 91));
            } else if (tile == 'S') {
                sf::CircleShape sw(13.f);
                sw.setOrigin({13.f, 13.f});
                sw.setPosition(pos + sf::Vector2f(TILE / 2.f, TILE / 2.f));
                sw.setFillColor(sf::Color(247, 196, 58));
                window.draw(sw);
            } else if (tile == 'X') {
                sf::ConvexShape spike(3);
                spike.setPoint(0, {TILE / 2.f, 10.f});
                spike.setPoint(1, {12.f, TILE - 12.f});
                spike.setPoint(2, {TILE - 12.f, TILE - 12.f});
                spike.setPosition(pos);
                spike.setFillColor(sf::Color(195, 66, 72));
                window.draw(spike);
            }
        }
    }
    const sf::Color wallColor(215, 220, 230);
    const float half = WALL_THICKNESS / 2.f;
    for (int y = 0; y < b.height; ++y) {
        for (int x = 0; x < b.width; ++x) {
            const float left = MARGIN + x * TILE;
            const float top = MARGIN + y * TILE;
            if (b.wall[y][x][UP])
                rect(window, {left - half, top - half},
                     {TILE + WALL_THICKNESS, WALL_THICKNESS}, wallColor);
            if (b.wall[y][x][LEFT])
                rect(window, {left - half, top - half},
                     {WALL_THICKNESS, TILE + WALL_THICKNESS}, wallColor);
            if (x == b.width - 1 && b.wall[y][x][RIGHT])
                rect(window, {left + TILE - half, top - half},
                     {WALL_THICKNESS, TILE + WALL_THICKNESS}, wallColor);
            if (y == b.height - 1 && b.wall[y][x][DOWN])
                rect(window, {left - half, top + TILE - half},
                     {TILE + WALL_THICKNESS, WALL_THICKNESS}, wallColor);
        }
    }
    drawCharacter(window, s.thief, sf::Color(67, 154, 244), s.controlThief);
    drawCharacter(window, s.police, sf::Color(240, 95, 99), !s.controlThief);
}

void updateTitle(sf::RenderWindow& window, const State& s) {
    std::string title = "Thief Puzzle | Control: ";
    title += s.controlThief ? "Thief" : "Police";
    if (s.result == Result::Win) title += " | WIN!";
    else if (s.result == Result::Lose) title += " | LOSE";
    if (!s.message.empty()) title += " | " + s.message;
    window.setTitle(title);
}
} // namespace

int main(int argc, char* argv[]) {
    const std::string filename = argc > 1 ? argv[1] : "stages/stage01.txt";
    Board board;
    State state{};
    if (!loadStage(filename, board, state)) return 1;
    const State initialState = state;
    std::vector<State> history;

    const unsigned width = static_cast<unsigned>(board.width * TILE + MARGIN * 2.f);
    const unsigned height = static_cast<unsigned>(board.height * TILE + MARGIN * 2.f);
    sf::RenderWindow window(sf::VideoMode({width, height}), "Thief Puzzle");
    window.setFramerateLimit(60);
    updateTitle(window, state);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                continue;
            }
            const auto* key = event->getIf<sf::Event::KeyPressed>();
            if (!key) continue;
            int direction = -1;
            switch (key->code) {
                case sf::Keyboard::Key::W: direction = UP; break;
                case sf::Keyboard::Key::D: direction = RIGHT; break;
                case sf::Keyboard::Key::S: direction = DOWN; break;
                case sf::Keyboard::Key::A: direction = LEFT; break;
                case sf::Keyboard::Key::Escape:
                case sf::Keyboard::Key::Q: window.close(); break;
                case sf::Keyboard::Key::R:
                    state = initialState;
                    history.clear();
                    state.message = "Stage restarted!";
                    break;
                case sf::Keyboard::Key::Z:
                    if (!history.empty()) {
                        state = history.back();
                        history.pop_back();
                        state.message = "Undid one move.";
                    } else state.message = "Nothing to undo!";
                    break;
                default: break;
            }
            if (direction != -1) {
                State previous = state;
                if (takeTurn(board, state, direction)) history.push_back(previous);
            }
            updateTitle(window, state);
        }
        window.clear(sf::Color(29, 32, 42));
        drawBoard(window, board, state);
        window.display();
    }
}
