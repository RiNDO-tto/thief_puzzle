#include "Game.hpp"
#include "Renderer.hpp"
#include "Menu.hpp"
#include "Solver.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <regex>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct StageEntry { std::string path; int world; int number; };
std::vector<StageEntry> findStages() {
    std::vector<StageEntry> result;
    if (!fs::exists("stages") || !fs::is_directory("stages")) return result;
    const std::regex pattern(R"(^stage([1-5])-([1-9][0-9]*)\.txt$)");
    for (const auto& entry : fs::directory_iterator("stages")) {
        if (!entry.is_regular_file()) continue;
        std::smatch match;
        const std::string name = entry.path().filename().string();
        if (std::regex_match(name, match, pattern))
            result.push_back({entry.path().string(), std::stoi(match[1]), std::stoi(match[2])});
    }
    std::sort(result.begin(), result.end(), [](const StageEntry& a, const StageEntry& b) {
        return std::tie(a.world, a.number) < std::tie(b.world, b.number);
    });
    return result;
}

int main(int argc, char* argv[]) {
#ifdef THIEF_PUZZLE_DEV
    constexpr bool developerMode = true;
#else
    constexpr bool developerMode = false;
#endif
    auto entries = findStages();
    std::vector<std::string> stages;
    for (const auto& entry : entries) stages.push_back(entry.path);
    std::array<int, 6> worldStarts{};
    for (int world = 1; world <= 5; ++world) {
        worldStarts[world - 1] = static_cast<int>(std::count_if(entries.begin(), entries.end(),
            [world](const StageEntry& e) { return e.world < world; }));
    }
    worldStarts[5] = static_cast<int>(entries.size());
    (void)argc; (void)argv; // Only numbered world stage files participate in progression.
    sf::RenderWindow window(sf::VideoMode({1024u, 720u}),
                            sf::String(developerMode ? std::u32string(U"Thief and Police [DEV]") : std::u32string(U"Thief and Police")));
#ifdef THIEF_PUZZLE_DEV
    std::cout << R"(
=== Thief & Police / Developer Mode ===
Stage selection (select a stage with arrows or WASD):
 F1  : mark thief objective cleared (official minimum)
 F2  : mark police objective cleared (official minimum)
 F3  : mark both available objectives optimal
 F4  : mark one available objective cleared in EVERY stage
 F5  : mark ALL available objectives cleared (non-optimal where possible)
 F6  : mark ALL available objectives optimal
 F7  : reset session progress and ending display history
 F8  : preview normal ending
 F9  : preview true ending
 F10 : preview perfect ending
Unavailable objectives are never marked cleared.
All changes are session-only. The normal executable has no developer shortcuts.
==========================================
)";
#endif
    const auto desktop = sf::VideoMode::getDesktopMode().size;
    window.setPosition({(static_cast<int>(desktop.x) - 1024) / 2,
                        (static_cast<int>(desktop.y) - 720) / 2});
    window.setFramerateLimit(60);
    Controls controls;
    bool fullscreen = false;
    bool vsync = false;
    int resolution = 0;
    const std::array<sf::Vector2u, 3> windowSizes = {{{1024u,720u},{1280u,800u},{800u,600u}}};
    // Recenter after recreating the OS window (fullscreen/resolution changes).
    auto centerWindow = [&]() {
        if (fullscreen) return;
        const sf::Vector2u screen = sf::VideoMode::getDesktopMode().size;
        const sf::Vector2u size = window.getSize();
        window.setPosition({(static_cast<int>(screen.x) - static_cast<int>(size.x)) / 2,
                            (static_cast<int>(screen.y) - static_cast<int>(size.y)) / 2});
    };
    auto applyDisplay = [&]() {
        const sf::VideoMode mode = fullscreen ? sf::VideoMode::getDesktopMode() : sf::VideoMode(windowSizes[resolution]);
        window.create(mode, sf::String(developerMode ? std::u32string(U"Thief and Police [DEV]") : std::u32string(U"Thief and Police")),
                      fullscreen ? sf::State::Fullscreen : sf::State::Windowed);
        window.setKeyRepeatEnabled(false);
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(vsync);
        window.setView(sf::View(sf::FloatRect({0.f,0.f},{1024.f,720.f})));
        centerWindow();
    };
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
    int selectedWorld = 0;
    int settingsSelected = 0;
    int keySelected = 0;
    int displaySelected = 0;
    bool capturingKey = false;
    std::string keyMessage;
    auto currentWorldStart = [&]() { return worldStarts[selectedWorld]; };
    auto currentWorldEnd = [&]() { return worldStarts[selectedWorld + 1]; };
    auto enterWorld = [&](int w) {
        selectedWorld = w;
        selected = worldStarts[w];
        screen = Screen::StageSelect;
    };
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
        window.setTitle(sf::String(developerMode ? std::u32string(U"Thief and Police [DEV]") : std::u32string(U"Thief and Police")));
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
        selectedWorld = entries[index].world - 1;
        hasStage = true;
        screen = Screen::Playing;
        updateTitle(window, state);
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) { window.close(); continue; }
            // Keep the window centered when the user manually resizes it.
            if (event->is<sf::Event::Resized>()) {
                centerWindow();
            }
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button == sf::Mouse::Button::Left) {
                    const sf::Vector2f p = window.mapPixelToCoords(mouse->position);
                    if (screen == Screen::Title) {
                        const int action = menu.titleClick(p);
                        if (action >= 0) titleSelected = action;
                        if (action == 0) screen = Screen::WorldSelect;
                        else if (action == 1) screen = Screen::HowTo;
                        else if (action == 2) screen = Screen::Settings;
                    }
                    else if (screen == Screen::WorldSelect) {
                        const int clicked = menu.worldClick(p);
                        if (clicked >= 0) enterWorld(clicked);
                    }
                    else if (screen == Screen::Settings) {
                        if (p.x>=250.f && p.x<=774.f) {
                            if (p.y>=210.f && p.y<=286.f) { settingsSelected=0; screen=Screen::KeySettings; }
                            else if (p.y>=318.f && p.y<=394.f) { settingsSelected=1; screen=Screen::DisplaySettings; }
                        }
                    }
                    else if (screen == Screen::KeySettings && !capturingKey) {
                        if (p.x>=190.f && p.x<=834.f && p.y>=112.f && p.y<112.f+9*54.f) {
                            int clicked = static_cast<int>((p.y-112.f)/54.f);
                            if (p.y<=112.f+clicked*54.f+46.f) {
                                keySelected=clicked;
                                if (keySelected==8) { controls=Controls{}; keyMessage="初期設定に戻しました"; }
                                else { capturingKey=true; keyMessage="変更するキーを押してください（Escでキャンセル）"; }
                            }
                        }
                    }
                    else if (screen == Screen::DisplaySettings) {
                        if (p.x>=175.f && p.x<=849.f && p.y>=202.f && p.y<202.f+3*108.f) {
                            int clicked = static_cast<int>((p.y-202.f)/108.f);
                            if (p.y<=202.f+clicked*108.f+76.f) {
                                displaySelected=clicked;
                                if (clicked==0) { fullscreen=!fullscreen; applyDisplay(); }
                                else if (clicked==1 && !fullscreen) { resolution=(resolution+1)%3; applyDisplay(); }
                                else if (clicked==2) { vsync=!vsync; window.setVerticalSyncEnabled(vsync); }
                            }
                        }
                    }
                    else if (screen == Screen::StageSelect) {
                        const int page = (selected - currentWorldStart()) / 16;
                        const int clicked = menu.stageClick(p, std::min(16, currentWorldEnd() - currentWorldStart() - page * 16));
                        if (clicked >= 0) {
                            const int index = currentWorldStart() + page * 16 + clicked;
                            if (isUnlocked(index)) { selected = index; openStage(index); }
                        }
                    }
                }
                continue;
            }
            const auto* key = event->getIf<sf::Event::KeyPressed>();
            if (!key) continue;
            const auto k = key->code;
            if (screen == Screen::KeySettings && capturingKey) {
                if (k == sf::Keyboard::Key::Escape) {
                    capturingKey = false; keyMessage = "変更をキャンセルしました";
                } else {
                    const bool ok = controls.assign(static_cast<Action>(keySelected), k);
                    keyMessage = ok ? "キーを変更しました" : "使用不可、または他の操作と重複しています";
                    if (ok) capturingKey = false;
                }
                continue;
            }
#ifdef THIEF_PUZZLE_DEV
            // Developer shortcuts are active only in the stage selection screen.
            // Preview endings do not alter the progression tier.
            if (screen == Screen::StageSelect) {
                auto markCleared = [](StageProgress& p, bool thief, bool optimal) {
                    const bool possible = thief ? p.thiefPossible : p.policePossible;
                    const int official = thief ? p.thiefOfficial : p.policeOfficial;
                    if (!possible || official < 0) return;
                    bool& cleared = thief ? p.thief : p.police;
                    int& best = thief ? p.thiefBest : p.policeBest;
                    cleared = true;
                    // For non-optimal completion, use official+1 if no record exists.
                    // Never overwrite a better existing record.
                    const int moves = optimal ? official : official + 1;
                    if (best < 0 || moves < best) best = moves;
                };
                bool handled = true;
                switch (k) {
                    case sf::Keyboard::Key::F1:
                        if (!progress.empty()) markCleared(progress[selected], true, true);
                        break;
                    case sf::Keyboard::Key::F2:
                        if (!progress.empty()) markCleared(progress[selected], false, true);
                        break;
                    case sf::Keyboard::Key::F3:
                        if (!progress.empty()) {
                            markCleared(progress[selected], true, true);
                            markCleared(progress[selected], false, true);
                        }
                        break;
                    case sf::Keyboard::Key::F4:
                        for (auto& p : progress) {
                            if (p.thiefPossible) markCleared(p, true, false);
                            else markCleared(p, false, false);
                        }
                        break;
                    case sf::Keyboard::Key::F5:
                        for (auto& p : progress) {
                            markCleared(p, true, false);
                            markCleared(p, false, false);
                        }
                        break;
                    case sf::Keyboard::Key::F6:
                        for (auto& p : progress) {
                            markCleared(p, true, true);
                            markCleared(p, false, true);
                        }
                        break;
                    case sf::Keyboard::Key::F7:
                        for (auto& p : progress) {
                            p.thief = p.police = false;
                            p.thiefBest = p.policeBest = -1;
                        }
                        highestEndingShown = -1;
                        selectedWorld = 0;
                        selected = 0;
                        break;
                    case sf::Keyboard::Key::F8:
                    case sf::Keyboard::Key::F9:
                    case sf::Keyboard::Key::F10:
                        currentEnding = (k == sf::Keyboard::Key::F8) ? EndingKind::Normal :
                                        (k == sf::Keyboard::Key::F9) ? EndingKind::True : EndingKind::Perfect;
                        screen = Screen::Ending;
                        break;
                    default: handled = false; break;
                }
                if (handled) continue;
            }
#endif
            if (screen == Screen::Title) {
                if (k == sf::Keyboard::Key::W || k == sf::Keyboard::Key::Up)
                    titleSelected = std::max(0, titleSelected - 1);
                else if (k == sf::Keyboard::Key::S || k == sf::Keyboard::Key::Down)
                    titleSelected = std::min(2, titleSelected + 1);
                else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) {
                    if (titleSelected == 0) screen = Screen::WorldSelect;
                    else if (titleSelected == 1) screen = Screen::HowTo;
                    else if (titleSelected == 2) screen = Screen::Settings;
                } else if (k == sf::Keyboard::Key::Escape) window.close();
            } else if (screen == Screen::WorldSelect) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Title;
                else if (k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::W) selectedWorld = std::max(0, selectedWorld - 1);
                else if (k == sf::Keyboard::Key::Down || k == sf::Keyboard::Key::S) selectedWorld = std::min(4, selectedWorld + 1);
                else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) enterWorld(selectedWorld);
            } else if (screen == Screen::Settings) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Title;
                else if (k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::W) settingsSelected = std::max(0, settingsSelected - 1);
                else if (k == sf::Keyboard::Key::Down || k == sf::Keyboard::Key::S) settingsSelected = std::min(1, settingsSelected + 1);
                else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space)
                    screen = settingsSelected == 0 ? Screen::KeySettings : Screen::DisplaySettings;
            } else if (screen == Screen::KeySettings) {
                if (k == sf::Keyboard::Key::Escape) { screen = Screen::Settings; keyMessage.clear(); }
                else if (k == sf::Keyboard::Key::Up) keySelected = std::max(0, keySelected - 1);
                else if (k == sf::Keyboard::Key::Down) keySelected = std::min(8, keySelected + 1);
                else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) {
                    if (keySelected == 8) { controls = Controls{}; keyMessage = "初期設定に戻しました"; }
                    else { capturingKey = true; keyMessage = "変更するキーを押してください（Escでキャンセル）"; }
                }
            } else if (screen == Screen::DisplaySettings) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Settings;
                else if (k == sf::Keyboard::Key::Up) displaySelected = std::max(0, displaySelected - 1);
                else if (k == sf::Keyboard::Key::Down) displaySelected = std::min(2, displaySelected + 1);
                else if (k == sf::Keyboard::Key::Left || k == sf::Keyboard::Key::Right || k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) {
                    if (displaySelected == 0) { fullscreen = !fullscreen; applyDisplay(); }
                    else if (displaySelected == 1 && !fullscreen) { resolution = (resolution + (k == sf::Keyboard::Key::Left ? 2 : 1)) % 3; applyDisplay(); }
                    else if (displaySelected == 2) { vsync = !vsync; window.setVerticalSyncEnabled(vsync); }
                }
            } else if (screen == Screen::HowTo) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::Title;
            } else if (screen == Screen::Ending) {
                if (k == sf::Keyboard::Key::Enter) screen = Screen::StageSelect;
            } else if (screen == Screen::StageSelect) {
                if (k == sf::Keyboard::Key::Escape) screen = Screen::WorldSelect;
                else if (currentWorldStart() < currentWorldEnd()) {
                    if (k == sf::Keyboard::Key::Left || k == sf::Keyboard::Key::A) selected = std::max(currentWorldStart(), selected - 1);
                    else if (k == sf::Keyboard::Key::Right || k == sf::Keyboard::Key::D) selected = std::min(currentWorldEnd() - 1, selected + 1);
                    else if (k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::W) selected = std::max(currentWorldStart(), selected - 4);
                    else if (k == sf::Keyboard::Key::Down || k == sf::Keyboard::Key::S) selected = std::min(currentWorldEnd() - 1, selected + 4);
                    else if (k == sf::Keyboard::Key::Enter || k == sf::Keyboard::Key::Space) openStage(selected);
                }
            } else if (screen == Screen::Playing && hasStage) {
                int direction = -1;
                if (controls.matches(Action::Up, k)) direction = UP;
                else if (controls.matches(Action::Right, k)) direction = RIGHT;
                else if (controls.matches(Action::Down, k)) direction = DOWN;
                else if (controls.matches(Action::Left, k)) direction = LEFT;
                else if (controls.matches(Action::Back, k)) returnToSelect();
                else if (controls.matches(Action::Next, k)) {
                    if (selected + 1 < static_cast<int>(stages.size()) && isUnlocked(selected + 1))
                        openStage(selected + 1);
                } else if (controls.matches(Action::Restart, k)) {
                    state = initialState;
                    history.clear();
                    state.message = "ステージをやり直しました！";
                } else if (controls.matches(Action::Undo, k)) {
                    if (!history.empty()) {
                        state = history.back(); history.pop_back();
                        state.message = "1手戻しました。";
                    } else state.message = "戻せる手がありません！";
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
        std::array<std::array<bool, 3>, 5> worldStatus{};
        for (int w=0;w<5;++w) {
            const int a=worldStarts[w], b=worldStarts[w+1];
            if (a==b) continue;
            worldStatus[w][0] = std::all_of(entries.begin()+a,entries.begin()+b,[&](const StageEntry& e){
                const int idx=static_cast<int>(&e - entries.data()); return isUnlocked(idx);
            });
            worldStatus[w][1] = std::all_of(progress.begin()+a,progress.begin()+b,[](const StageProgress& p){
                return (p.thiefPossible || p.policePossible) && (!p.thiefPossible || p.thief) && (!p.policePossible || p.police);
            });
            worldStatus[w][2] = std::all_of(progress.begin()+a,progress.begin()+b,[](const StageProgress& p){
                return (p.thiefPossible || p.policePossible) && (!p.thiefPossible || p.thiefBest==p.thiefOfficial) && (!p.policePossible || p.policeBest==p.policeOfficial);
            });
        }
        if (screen == Screen::Title) menu.drawTitle(window, titleSelected);
        else if (screen == Screen::WorldSelect) menu.drawWorldSelect(window, selectedWorld, worldStatus);
        else if (screen == Screen::Settings) menu.drawSettings(window, settingsSelected);
        else if (screen == Screen::KeySettings) menu.drawKeySettings(window, controls, keySelected, capturingKey, keyMessage);
        else if (screen == Screen::DisplaySettings) menu.drawDisplaySettings(window, displaySelected, fullscreen, resolution, vsync);
        else if (screen == Screen::StageSelect) menu.drawStageSelect(window, stages, selected, progress, selectedWorld+1, currentWorldStart(), currentWorldEnd());
        else if (screen == Screen::HowTo) menu.drawHowTo(window, controls);
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
            drawHud(window, board, state, history.size(), currentFile, progress[selected], controls, isUnlocked(selected+1));
            window.setView(oldView);
            menu.drawPlayingResult(window, state, controls, isUnlocked(selected+1));
        }
        window.display();
    }
}
