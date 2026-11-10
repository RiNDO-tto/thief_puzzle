#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Game.hpp"
#include "Controls.hpp"
#include <vector>

enum class Screen { Title, WorldSelect, StageSelect, Playing, Ending, HowTo, Settings, KeySettings, DisplaySettings };
enum class EndingKind { Normal, True, Perfect };

struct StageProgress {
    bool thief = false;
    bool police = false;
    bool thiefPossible = true;
    bool policePossible = true;
    int thiefOfficial = -1;
    int policeOfficial = -1;
    int thiefBest = -1;
    int policeBest = -1;
};

class Menu {
public:
    Menu();
    void drawEnding(sf::RenderWindow& window, EndingKind kind) const;
    void drawPlayingResult(sf::RenderWindow& window, const State& state, const Controls& controls, bool nextUnlocked) const;
    void drawWorldSelect(sf::RenderWindow& window, int selectedWorld, const std::array<std::array<bool, 3>, 5>& status) const;
    void drawSettings(sf::RenderWindow& window, int selected) const;
    void drawKeySettings(sf::RenderWindow& window, const Controls& controls, int selected, bool capturing, const std::string& message) const;
    void drawDisplaySettings(sf::RenderWindow& window, int selected, bool fullscreen, int resolution, bool vsync) const;
    void drawTitle(sf::RenderWindow& window, int selected) const;
    void drawHowTo(sf::RenderWindow& window, const Controls& controls) const;
    void drawStageSelect(sf::RenderWindow& window, const std::vector<std::string>& stages, int selected,
                         const std::vector<StageProgress>& progress, int world, int worldStart, int worldEnd) const;
    // Returns: title 0=Start, 1=Settings; stage select index; -1=none.
    int titleClick(sf::Vector2f pos) const;
    int stageClick(sf::Vector2f pos, int count) const;
    int worldClick(sf::Vector2f pos) const;
private:
    sf::Font font_;
    bool fontReady_ = false;
    mutable bool cursorReady_ = false;
    mutable sf::Vector2f cursorPosition_{0.f, 0.f};
    mutable sf::Clock cursorClock_;
    void text(sf::RenderWindow& w, const std::string& s, unsigned size, sf::Vector2f p,
              sf::Color color, bool centered = true) const;
    void button(sf::RenderWindow& w, sf::FloatRect bounds, const std::string& label, bool selected = false) const;
};
