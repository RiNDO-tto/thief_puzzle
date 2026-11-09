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
        "assets/fonts/NotoSansCJKjp-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/mnt/c/Windows/Fonts/meiryo.ttc",
        "/mnt/c/Windows/Fonts/YuGothR.ttc",
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
    sf::Text t(font_, sf::String::fromUtf8(s.begin(), s.end()), size);
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
    text(w, "泥棒と警察", 57, {WIDTH / 2.f, 170.f}, ink);
    text(w, "ふたつの勝利を目指すパズル", 24, {WIDTH / 2.f, 247.f}, ink);
    button(w, {{BUTTON_X, 393.f}, {BUTTON_W, BUTTON_H}}, "はじめる", true);
    button(w, {{BUTTON_X, 492.f}, {BUTTON_W, BUTTON_H}}, "設定（準備中）");
    text(w, "Enter：はじめる　 Esc：終了", 18, {WIDTH / 2.f, 648.f}, ink);
}
void Menu::drawStageSelect(sf::RenderWindow& w, const std::vector<std::string>& stages, int selected,
                           const std::vector<StageProgress>& progress) const {
    w.clear(sf::Color(179, 216, 198));
    text(w, "ステージ選択", 48, {WIDTH / 2.f, 78.f}, ink);
    text(w, "矢印・WASD：選択　 Enter：開始　 Esc：戻る", 18, {WIDTH / 2.f, 135.f}, ink);
    if (stages.empty()) {
        text(w, "ステージファイルがありません", 30, {WIDTH / 2.f, 350.f}, ink);
        return;
    }
    // 4 columns, 4 rows per page; completion and locks are session-only.
    text(w, "左の光：泥棒の脱出　　右の光：警察の逮捕", 17,
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
        text(w, unlocked ? name : name + " 鍵", 23, {x + 90.f, y + 22.f}, ink);
        auto lamp = [&](float cx, bool lit, sf::Color on) {
            sf::CircleShape glow(15.f);
            glow.setOrigin({15.f, 15.f});
            glow.setPosition({cx, y + 62.f});
            glow.setFillColor(lit ? sf::Color(on.r, on.g, on.b, 65) : sf::Color::Transparent);
            w.draw(glow);
            sf::CircleShape bulb(9.f);
            bulb.setOrigin({9.f, 9.f});
            bulb.setPosition({cx, y + 62.f});
            bulb.setFillColor(lit ? on : sf::Color(92, 97, 96));
            bulb.setOutlineColor(lit ? sf::Color::White : sf::Color(65, 70, 70));
            bulb.setOutlineThickness(2.f);
            w.draw(bulb);
        };
        lamp(x + 67.f, thiefDone, sf::Color(255, 210, 69));
        lamp(x + 113.f, policeDone, sf::Color(83, 191, 255));
    }
    text(w, "ページ " + std::to_string(page + 1) + " / " + std::to_string((stages.size() + 15) / 16),
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

void Menu::drawEnding(sf::RenderWindow& w) const {
    w.clear(sf::Color(179, 216, 198));
    block(w, {{110.f, 120.f}, {804.f, 440.f}}, cream);
    text(w, "全ステージクリア！", 52, {512.f, 245.f}, ink);
    text(w, "おめでとうございます！", 34, {512.f, 340.f}, ink);
    text(w, "Enterキーでステージ選択へ", 23, {512.f, 465.f}, ink);
}
void Menu::drawPlayingResult(sf::RenderWindow& w, const State& s) const {
    if (s.result == Result::Playing) return;
    block(w, {{105.f, 278.f}, {610.f, 166.f}}, sf::Color(32, 40, 52));
    text(w, s.result == Result::Win ? "目標達成！" : "失敗！", 48,
         {410.f, 345.f}, s.result == Result::Win ? sf::Color(126, 234, 157) : sf::Color(255, 141, 143));
    text(w, "Q：ステージ選択へ　 R：やり直す", 19, {410.f, 402.f}, sf::Color::White);
}
