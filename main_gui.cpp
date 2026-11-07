// SFML 3 frontend for the existing console simulator.
// Keep main2-2.cpp in the same directory. Its game rules are reused unchanged.
#define main console_simulator_main
#include "main2-2.cpp"
#undef main

#include <SFML/Graphics.hpp>
#include <optional>
#include <algorithm>
#include <cmath>
#include <cstdint>

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

// Draw large WIN / LOSE letters without requiring an external font file.
void drawPixelLetter(sf::RenderWindow& window, char letter,
                     sf::Vector2f pos, float pixel, sf::Color color) {
    const char* rows[7] = {};
    static const char* W[7] = {"10001", "10001", "10001", "10101", "10101", "11011", "10001"};
    static const char* I[7] = {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
    static const char* N[7] = {"10001", "11001", "11001", "10101", "10011", "10011", "10001"};
    static const char* L[7] = {"10000", "10000", "10000", "10000", "10000", "10000", "11111"};
    static const char* O[7] = {"01110", "10001", "10001", "10001", "10001", "10001", "01110"};
    static const char* S[7] = {"01111", "10000", "10000", "01110", "00001", "00001", "11110"};
    static const char* E[7] = {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
    const char* const* glyph = nullptr;
    switch (letter) {
        case 'W': glyph = W; break;
        case 'I': glyph = I; break;
        case 'N': glyph = N; break;
        case 'L': glyph = L; break;
        case 'O': glyph = O; break;
        case 'S': glyph = S; break;
        case 'E': glyph = E; break;
        default: return;
    }
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < 5; ++x)
            if (glyph[y][x] == '1')
                rect(window, pos + sf::Vector2f(x * pixel, y * pixel),
                     {pixel * .86f, pixel * .86f}, color);
}

void drawResultOverlay(sf::RenderWindow& window, const State& s) {
    if (s.result == Result::Playing) return;
    const sf::Vector2f size(window.getSize());
    rect(window, {0.f, 0.f}, size, sf::Color(0, 0, 0, 165));

    const bool won = s.result == Result::Win;
    const sf::Color accent = won ? sf::Color(65, 210, 130) : sf::Color(250, 100, 110);
    const float bandHeight = std::min(160.f, size.y * .52f);
    rect(window, {0.f, (size.y - bandHeight) / 2.f},
         {size.x, bandHeight}, sf::Color(20, 24, 34, 245));
    rect(window, {0.f, (size.y - bandHeight) / 2.f}, {size.x, 5.f}, accent);
    rect(window, {0.f, (size.y + bandHeight) / 2.f - 5.f}, {size.x, 5.f}, accent);

    const std::string word = won ? "WIN" : "LOSE";
    const float pixel = std::min({10.f, (size.x - 30.f) / (word.size() * 6.f - 1.f),
                                  (bandHeight - 25.f) / 7.f});
    const float wordWidth = (word.size() * 6.f - 1.f) * pixel;
    sf::Vector2f start((size.x - wordWidth) / 2.f, (size.y - 7.f * pixel) / 2.f);
    for (std::size_t i = 0; i < word.size(); ++i)
        drawPixelLetter(window, word[i], start + sf::Vector2f(i * 6.f * pixel, 0.f),
                        pixel, accent);
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
    // Place the window in the center of the primary desktop.
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    window.setPosition({
        (static_cast<int>(desktop.x) - static_cast<int>(width)) / 2,
        (static_cast<int>(desktop.y) - static_cast<int>(height)) / 2
    });
    window.setKeyRepeatEnabled(false);
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
        drawResultOverlay(window, state);
        window.display();
    }
}
