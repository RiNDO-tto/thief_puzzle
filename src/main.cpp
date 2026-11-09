#include "Game.hpp"
#include "Renderer.hpp"
#include "Menu.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> findStages() {
    std::vector<std::string> result;
    if (!fs::exists("stages") || !fs::is_directory("stages")) return result;
    for (const auto& entry : fs::directory_iterator("stages")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
        const auto name = entry.path().filename().string();
        // Ignore stageXX_fixed.txt and other experimental stage files.
        if (name.rfind("stage", 0) != 0 || name.find("fixed") != std::string::npos) continue;
        result.push_back(entry.path().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

int main(int argc, char* argv[]) {
    auto stages = findStages();
    if (argc > 1) {
        const std::string extra = argv[1];
        if (std::find(stages.begin(), stages.end(), extra) == stages.end()) stages.insert(stages.begin(), extra);
    }
    sf::RenderWindow window(sf::VideoMode({1024u, 720u}), "Thief Puzzle");
    const auto desktop = sf::VideoMode::getDesktopMode().size;
    window.setPosition({(static_cast<int>(desktop.x) - 1024) / 2,
                        (static_cast<int>(desktop.y) - 720) / 2});
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    Menu menu;
    Screen screen = Screen::Title;
    Board board;
    State state{};
    State initialState{};
    std::vector<State> history;
    std::string currentFile;
    int selected = 0;
    bool hasStage = false;
    // Session-only completion flags: no save file is written.
    std::vector<StageProgress> progress(stages.size());
    auto isUnlocked = [&](int index) {
        if (index < 0 || index >= static_cast<int>(stages.size())) return false;
        if (index == 0) return true;
        // Clearing either objective in the preceding stage unlocks this one.
        return progress[index - 1].thief || progress[index - 1].police;
    };

    auto openStage = [&](int index) {
        if (!isUnlocked(index)) return;
        Board loadedBoard;
        State loadedState{};
        if (!loadStage(stages[index], loadedBoard, loadedState)) {
            std::cerr << "Could not load stage: " << stages[index] << '\n';
            return;
        }
        board = std::move(loadedBoard);
        state = std::move(loadedState);
        initialState = state;
        history.clear();
        currentFile = stages[index];
        selected = index;
        hasStage = true;
        screen = Screen::Playing;
        updateTitle(window, state);
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) { window.close(); continue; }
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button == sf::Mouse::Button::Left) {
                    const sf::Vector2f p = window.mapPixelToCoords(mouse->position);
                    if (screen == Screen::Title && menu.titleClick(p) == 0) screen = Screen::StageSelect;
                    else if (screen == Screen::StageSelect) {
                        const int page = selected / 16;
                        const int clicked = menu.stageClick(p, std::min(16, static_cast<int>(stages.size()) - page * 16));
                        if (clicked >= 0) {
                            const int index = page * 16 + clicked;
                            if (isUnlocked(index)) { selected = index; openStage(index); }
                        }
                    }
                }
                continue;
            }
            const auto* key = event->getIf<sf::Event::KeyPressed>();
            if (!key) continue;
            const auto k = key->code;
            if (screen == Screen::Title) {
                if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) screen = Screen::StageSelect;
                else if (k == sf::Keyboard::Key::Escape) window.close();
            } else if (screen == Screen::StageSelect) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Title;
                else if (!stages.empty()) {
                    if (k == sf::Keyboard::Key::Left || k == sf::Keyboard::Key::A) selected = std::max(0, selected - 1);
                    else if (k == sf::Keyboard::Key::Right || k == sf::Keyboard::Key::D) selected = std::min(static_cast<int>(stages.size()) - 1, selected + 1);
                    else if (k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::W) selected = std::max(0, selected - 4);
                    else if (k == sf::Keyboard::Key::Down || k == sf::Keyboard::Key::S) selected = std::min(static_cast<int>(stages.size()) - 1, selected + 4);
                    else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) openStage(selected);
                }
            } else if (screen == Screen::Playing && hasStage) {
                int direction = -1;
                switch (k) {
                    case sf::Keyboard::Key::W: direction = UP; break;
                    case sf::Keyboard::Key::D: direction = RIGHT; break;
                    case sf::Keyboard::Key::S: direction = DOWN; break;
                    case sf::Keyboard::Key::A: direction = LEFT; break;
                    case sf::Keyboard::Key::Q:
                        screen = Screen::StageSelect;
                        window.setTitle("Thief Puzzle");
                        break;
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
                    if (takeTurn(board, state, direction)) {
                        history.push_back(previous);
                        if (previous.result == Result::Playing && state.result == Result::Win) {
                            // A win belongs to the side controlled at the winning move.
                            if (previous.controlThief) progress[selected].thief = true;
                            else progress[selected].police = true;
                        }
                    }
                }
                if (screen == Screen::Playing) updateTitle(window, state);
            }
        }
        if (screen == Screen::Title) menu.drawTitle(window);
        else if (screen == Screen::StageSelect) menu.drawStageSelect(window, stages, selected, progress);
        else {
            window.clear(sf::Color(29, 32, 42));
            drawBoard(window, board, state);
            drawHud(window, board, state, history.size(), currentFile);
            drawResultOverlay(window, state);
        }
        window.display();
    }
}
