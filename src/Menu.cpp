#include "Menu.hpp"
#include <algorithm>
#include <filesystem>

namespace {
constexpr float WIDTH = 1024.f;
constexpr float BUTTON_X = 342.f;
constexpr float BUTTON_W = 340.f;
constexpr float BUTTON_H = 72.f;
const sf::Color ink(36, 54, 67);
const sf::Color cream(251, 242, 218);
const sf::Color green(118, 167, 117);
void block(sf::RenderWindow& w, sf::FloatRect r, sf::Color fill, sf::Color outline = ink) {
    sf::RectangleShape shape({r.size.x, r.size.y});
    shape.setPosition(r.position);
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(4.f);
    w.draw(shape);
}
}

Menu::Menu() {
    const char* paths[] = {
        "assets/fonts/menu.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
    };
    for (auto path : paths) {
        if (std::filesystem::exists(path) && font_.openFromFile(path)) {
            fontReady_ = true;
            break;
        }
    }
}
void Menu::text(sf::RenderWindow& w, const std::string& s, unsigned size,
                sf::Vector2f p, sf::Color color, bool centered) const {
    if (!fontReady_) return;
    sf::Text t(font_, s, size);
    t.setFillColor(color);
    const auto b = t.getLocalBounds();
    if (centered) t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    t.setPosition(p);
    w.draw(t);
}
void Menu::button(sf::RenderWindow& w, sf::FloatRect r, const std::string& label, bool selected) const {
    block(w, r, selected ? sf::Color(252, 212, 108) : cream);
    text(w, label, 30, {r.position.x + r.size.x / 2.f, r.position.y + r.size.y / 2.f}, ink);
}
void Menu::drawTitle(sf::RenderWindow& w) const {
    w.clear(sf::Color(179, 216, 198));
    // Hand-drawn-style placeholder: big title panel and simple geometric decoration.
    block(w, {{122.f, 88.f}, {780.f, 225.f}}, cream);
    text(w, "THIEF & POLICE", 57, {WIDTH / 2.f, 170.f}, ink);
    text(w, "A TWO-WAY PUZZLE", 24, {WIDTH / 2.f, 247.f}, ink);
    button(w, {{BUTTON_X, 393.f}, {BUTTON_W, BUTTON_H}}, "START", true);
    button(w, {{BUTTON_X, 492.f}, {BUTTON_W, BUTTON_H}}, "SETTINGS (SOON)");
    text(w, "ENTER: START   |   ESC: QUIT", 18, {WIDTH / 2.f, 648.f}, ink);
}
void Menu::drawStageSelect(sf::RenderWindow& w, const std::vector<std::string>& stages, int selected,
                           const std::vector<StageProgress>& progress) const {
    w.clear(sf::Color(179, 216, 198));
    text(w, "CHOOSE A STAGE", 48, {WIDTH / 2.f, 78.f}, ink);
    text(w, "ARROWS / WASD: SELECT     ENTER: PLAY     ESC: BACK", 18, {WIDTH / 2.f, 135.f}, ink);
    if (stages.empty()) {
        text(w, "No stages found in stages/", 30, {WIDTH / 2.f, 350.f}, ink);
        return;
    }
    // 4 columns, 4 rows per page; completion and locks are session-only.
    text(w, "T = THIEF ESCAPE     P = POLICE ARREST     CHECK = CLEARED", 17,
         {WIDTH / 2.f, 161.f}, ink);
    const int page = std::max(0, selected) / 16;
    const int start = page * 16;
    const int end = std::min(static_cast<int>(stages.size()), start + 16);
    for (int i = start; i < end; ++i) {
        const int local = i - start;
        const float x = 105.f + (local % 4) * 210.f;
        const float y = 190.f + (local / 4) * 103.f;
        const std::string name = "1-" + std::to_string(i + 1);
        const bool unlocked = (i == 0) ||
            (i - 1 < static_cast<int>(progress.size()) &&
             (progress[i - 1].thief || progress[i - 1].police));
        const bool thiefDone = i < static_cast<int>(progress.size()) && progress[i].thief;
        const bool policeDone = i < static_cast<int>(progress.size()) && progress[i].police;
        block(w, {{x, y}, {180.f, 86.f}},
              unlocked ? (i == selected ? sf::Color(252, 212, 108) : cream)
                       : sf::Color(177, 182, 176));
        text(w, unlocked ? name : name + " LOCK", 23, {x + 90.f, y + 22.f}, ink);
        text(w, std::string("T:") + (thiefDone ? "CHECK" : "--") +
                    "   P:" + (policeDone ? "CHECK" : "--"),
             17, {x + 90.f, y + 61.f},
             unlocked ? ink : sf::Color(98, 103, 102));
    }
    text(w, "PAGE " + std::to_string(page + 1) + " / " + std::to_string((stages.size() + 15) / 16),
         20, {WIDTH / 2.f, 654.f}, ink);
}
int Menu::titleClick(sf::Vector2f p) const {
    if (p.x >= BUTTON_X && p.x <= BUTTON_X + BUTTON_W) {
        if (p.y >= 393.f && p.y <= 393.f + BUTTON_H) return 0;
        if (p.y >= 492.f && p.y <= 492.f + BUTTON_H) return 1;
    }
    return -1;
}
int Menu::stageClick(sf::Vector2f p, int count) const {
    for (int i = 0; i < count && i < 16; ++i) {
        const float x = 105.f + (i % 4) * 210.f;
        const float y = 190.f + (i / 4) * 103.f;
        if (p.x >= x && p.x <= x + 180.f && p.y >= y && p.y <= y + 76.f) return i;
    }
    return -1;
}
