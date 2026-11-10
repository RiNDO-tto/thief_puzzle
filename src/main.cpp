#include "Game.hpp"
#include "Renderer.hpp"
#include "Menu.hpp"
#include "Solver.hpp"
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
    sf::RenderWindow window(sf::VideoMode({1024u, 720u}), "泥棒と警察");
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
    int titleSelected = 0;
    bool hasStage = false;
    int highestEndingShown = -1; // -1: none, 0: normal, 1: true, 2: perfect
    EndingKind currentEnding = EndingKind::Normal;
    // Session-only completion flags: no save file is written.
    std::vector<StageProgress> progress(stages.size());
    // Check each stage once at startup, using exactly the game's movement rules.
    for (std::size_t i = 0; i < stages.size(); ++i) {
        Board candidate;
        State initial{};
        if (loadStage(stages[i], candidate, initial)) {
            const auto shortest = solveShortestMoves(candidate, initial);
            progress[i].thiefOfficial = shortest.thief;
            progress[i].policeOfficial = shortest.police;
            progress[i].thiefPossible = shortest.thief >= 0;
            progress[i].policePossible = shortest.police >= 0;
        } else {
            progress[i].thiefPossible = false;
            progress[i].policePossible = false;
        }
    }
    auto isUnlocked = [&](int index) {
        if (index < 0 || index >= static_cast<int>(stages.size())) return false;
        if (index == 0) return true;
        // Clearing either objective in the preceding stage unlocks this one.
        return progress[index - 1].thief || progress[index - 1].police;
    };

    auto allCleared = [&]() {
        return !progress.empty() && std::all_of(progress.begin(), progress.end(),
            [](const StageProgress& p) { return p.thief || p.police; });
    };
    // Objectives with no BFS solution are not required for higher endings.
    // A stage with no solvable objective cannot count as fully completed.
    auto allBothCleared = [&]() {
        return !progress.empty() && std::all_of(progress.begin(), progress.end(),
            [](const StageProgress& p) {
                const bool hasObjective = p.thiefPossible || p.policePossible;
                return hasObjective &&
                       (!p.thiefPossible || p.thief) &&
                       (!p.policePossible || p.police);
            });
    };
    auto allOptimal = [&]() {
        return !progress.empty() && std::all_of(progress.begin(), progress.end(),
            [](const StageProgress& p) {
                const bool hasObjective = p.thiefPossible || p.policePossible;
                return hasObjective &&
                       (!p.thiefPossible || (p.thiefBest >= 0 && p.thiefBest == p.thiefOfficial)) &&
                       (!p.policePossible || (p.policeBest >= 0 && p.policeBest == p.policeOfficial));
            });
    };
    auto returnToSelect = [&]() {
        // Show only an ending that exceeds the highest tier shown this session.
        // If multiple tiers become eligible at once, show only the highest.
        int tier = -1;
        if (allOptimal()) tier = 2;
        else if (allBothCleared()) tier = 1;
        else if (allCleared()) tier = 0;
        if (tier > highestEndingShown) {
            highestEndingShown = tier;
            currentEnding = static_cast<EndingKind>(tier);
            screen = Screen::Ending;
        } else {
            screen = Screen::StageSelect;
        }
        window.setTitle(sf::String(L"泥棒と警察"));
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
                    if (screen == Screen::Title) {
                        const int action = menu.titleClick(p);
                        if (action >= 0) titleSelected = action;
                        if (action == 0) screen = Screen::StageSelect;
                        else if (action == 1) screen = Screen::HowTo;
                    }
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
                if (k == sf::Keyboard::Key::W || k == sf::Keyboard::Key::Up)
                    titleSelected = std::max(0, titleSelected - 1);
                else if (k == sf::Keyboard::Key::S || k == sf::Keyboard::Key::Down)
                    titleSelected = std::min(2, titleSelected + 1);
                else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) {
                    if (titleSelected == 0) screen = Screen::StageSelect;
                    else if (titleSelected == 1) screen = Screen::HowTo;
                } else if (k == sf::Keyboard::Key::Escape) window.close();
            } else if (screen == Screen::HowTo) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Title;
            } else if (screen == Screen::Ending) {
                if (k == sf::Keyboard::Key::Enter) screen = Screen::StageSelect;
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
                        returnToSelect();
                        break;
                    case sf::Keyboard::Key::R:
                        state = initialState;
                        history.clear();
                        state.message = "ステージをやり直しました！";
                        break;
                    case sf::Keyboard::Key::Z:
                        if (!history.empty()) {
                            state = history.back();
                            history.pop_back();
                            state.message = "1手戻しました。";
                        } else state.message = "戻せる手がありません！";
                        break;
                    default: break;
                }
                if (direction != -1) {
                    State previous = state;
                    if (takeTurn(board, state, direction)) {
                        history.push_back(previous);
                        if (previous.result == Result::Playing && state.result == Result::Win) {
                            // A win belongs to the side controlled at the winning move.
                            StageProgress& p = progress[selected];
                            const int moves = static_cast<int>(history.size());
                            if (previous.controlThief) {
                                p.thief = true;
                                if (p.thiefBest < 0 || moves < p.thiefBest) p.thiefBest = moves;
                            } else {
                                p.police = true;
                                if (p.policeBest < 0 || moves < p.policeBest) p.policeBest = moves;
                            }
                        }
                    }
                }
                if (screen == Screen::Playing) updateTitle(window, state);
            }
        }
        if (screen == Screen::Title) menu.drawTitle(window, titleSelected);
        else if (screen == Screen::StageSelect) menu.drawStageSelect(window, stages, selected, progress);
        else if (screen == Screen::HowTo) menu.drawHowTo(window);
        else if (screen == Screen::Ending) menu.drawEnding(window, currentEnding);
        else {
            window.clear(sf::Color(29, 32, 42));
            // 70% for the board, 30% for the instructions.
            const sf::View oldView = window.getView();
            const float boardW = board.width * 76.f + 60.f;
            const float boardH = board.height * 76.f + 60.f;
            const float scale = std::max(boardW / (1024.f * .7f), boardH / 720.f);
            sf::View boardView(sf::FloatRect({0.f, 0.f}, {1024.f * .7f * scale, 720.f * scale}));
            boardView.setCenter({boardW / 2.f, boardH / 2.f});
            boardView.setViewport(sf::FloatRect({0.f, 0.f}, {.7f, 1.f}));
            window.setView(boardView);
            drawBoard(window, board, state);
            // HUD retains its original 268-unit layout in a separate viewport.
            const float hudX = 60.f + board.width * 76.f + 16.f;
            sf::View hudView(sf::FloatRect({hudX - 4.f, 0.f}, {300.f, 720.f}));
            hudView.setViewport(sf::FloatRect({.7f, 0.f}, {.3f, 1.f}));
            window.setView(hudView);
            drawHud(window, board, state, history.size(), currentFile, progress[selected]);
            window.setView(oldView);
            menu.drawPlayingResult(window, state);
        }
        window.display();
    }
}
