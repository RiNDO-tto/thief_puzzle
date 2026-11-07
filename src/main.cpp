#include "Game.hpp"
#include "Renderer.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    const std::string filename = argc > 1 ? argv[1] : "stages/stage01.txt";
    Board board;
    State state{};
    if (!loadStage(filename, board, state)) return 1;
    const State initialState = state;
    std::vector<State> history;

    const unsigned width = static_cast<unsigned>(board.width * 76.f + 30.f * 2.f);
    const unsigned height = static_cast<unsigned>(board.height * 76.f + 30.f * 2.f);
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
