#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Game.hpp"
#include <vector>

enum class Screen { Title, StageSelect, Playing, Ending, HowTo };

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
    void drawEnding(sf::RenderWindow& window) const;
    void drawPlayingResult(sf::RenderWindow& window, const State& state) const;
    void drawTitle(sf::RenderWindow& window, int selected) const;
    void drawHowTo(sf::RenderWindow& window) const;
    void drawStageSelect(sf::RenderWindow& window, const std::vector<std::string>& stages, int selected,
                         const std::vector<StageProgress>& progress) const;
    // Returns: title 0=Start, 1=Settings; stage select index; -1=none.
    int titleClick(sf::Vector2f pos) const;
    int stageClick(sf::Vector2f pos, int count) const;
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
