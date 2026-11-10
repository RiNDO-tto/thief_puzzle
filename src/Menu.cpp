#include "Menu.hpp"
#include <algorithm>
#include <filesystem>
#include <cmath>

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

void rectCorner(sf::RenderWindow& w, float x, float y, int right, int bottom,
                float arm, float thick, sf::Color color) {
    sf::RectangleShape horizontal({arm, thick});
    horizontal.setPosition({right ? x - arm : x, bottom ? y - thick : y});
    horizontal.setFillColor(color);
    w.draw(horizontal);
    sf::RectangleShape vertical({thick, arm});
    vertical.setPosition({right ? x - thick : x, bottom ? y - arm : y});
    vertical.setFillColor(color);
    w.draw(vertical);
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
void Menu::drawTitle(sf::RenderWindow& w, int selected) const {
    w.clear(sf::Color(179, 216, 198));
    // Hand-drawn-style placeholder: big title panel and simple geometric decoration.
    block(w, {{122.f, 88.f}, {780.f, 225.f}}, cream);
    text(w, "Thief and Police", 57, {WIDTH / 2.f, 170.f}, ink);
    text(w, "僕たち（私たち）入れ替わってる!?", 24, {WIDTH / 2.f, 247.f}, ink);
    button(w, {{BUTTON_X, 356.f}, {BUTTON_W, 68.f}}, "スタート", selected == 0);
    button(w, {{BUTTON_X, 445.f}, {BUTTON_W, 68.f}}, "ゲーム説明", selected == 1);
    button(w, {{BUTTON_X, 534.f}, {BUTTON_W, 68.f}}, "設定（準備中）", selected == 2);
    text(w, "W/S・↑/↓：選択　 Enter：決定　 Esc：終了", 18, {WIDTH / 2.f, 665.f}, ink);
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
    // Compact per-stage progress: completion and optimality are independent.
    text(w, "黄：泥棒  青：警察   二重丸：最短達成   数字：自己ベスト / 公式最短", 16,
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
        block(w, {{x, y}, {180.f, 86.f}},
              unlocked ? cream : sf::Color(177, 182, 176));
        text(w, name, 22, {x + 90.f, y + 17.f}, unlocked ? ink : sf::Color(96, 100, 99));
        if (i >= static_cast<int>(progress.size())) continue;
        const auto& p = progress[i];
        auto progressLine = [&](float cy, const char* side, bool possible, bool done,
                                int best, int official, sf::Color color) {
            if (!possible) return;
            const bool optimal = done && official >= 0 && best == official;
            // The inner lamp shows objective completion; the outer ring shows optimality.
            const sf::Vector2f center{x + 16.f, cy};
            if (optimal) {
                sf::CircleShape ring(9.f);
                ring.setOrigin({9.f, 9.f});
                ring.setPosition(center);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor(color);
                ring.setOutlineThickness(2.f);
                w.draw(ring);
            }
            sf::CircleShape bulb(5.f);
            bulb.setOrigin({5.f, 5.f});
            bulb.setPosition(center);
            bulb.setFillColor(done ? color : sf::Color(100, 104, 102));
            bulb.setOutlineColor(ink);
            bulb.setOutlineThickness(1.f);
            w.draw(bulb);
            const std::string score = (best >= 0 ? std::to_string(best) : "--") +
                                      " / " + (official >= 0 ? std::to_string(official) : "--");
            text(w, std::string(side) + " " + score,
                 13, {x + 30.f, cy - 10.f}, unlocked ? ink : sf::Color(96, 100, 99), false);
        };
        progressLine(y + 46.f, "泥棒", p.thiefPossible, p.thief, p.thiefBest,
                     p.thiefOfficial, sf::Color(244, 189, 46));
        progressLine(y + 69.f, "警察", p.policePossible, p.police, p.policeBest,
                     p.policeOfficial, sf::Color(83, 191, 255));
    }
    // Four L-shaped corner brackets glide toward the selected stage.
    const int localSelected = selected - start;
    const sf::Vector2f target{105.f + (localSelected % 4) * 210.f,
                              190.f + (localSelected / 4) * 103.f};
    if (!cursorReady_) { cursorPosition_ = target; cursorReady_ = true; cursorClock_.restart(); }
    const float dt = std::min(cursorClock_.restart().asSeconds(), 0.05f);
    const float blend = 1.f - std::exp(-13.f * dt);
    cursorPosition_ += (target - cursorPosition_) * blend;
    const float x = cursorPosition_.x - 7.f;
    const float y = cursorPosition_.y - 7.f;
 
    constexpr float markerW = 194.f;
    constexpr float h = 100.f;
    constexpr float arm = 23.f;
    constexpr float thick = 5.f;

    const sf::Color marker(47, 85, 145);

    for (int side = 0; side < 2; ++side) {
        for (int vertical = 0; vertical < 2; ++vertical) {
            const float px = x + (side ? markerW : 0.f);
            const float py = y + (vertical ? h : 0.f);
            rectCorner(w, px, py, side, vertical, arm, thick, marker);
        }
    }
    text(w, "ページ " + std::to_string(page + 1) + " / " + std::to_string((stages.size() + 15) / 16),
         20, {WIDTH / 2.f, 654.f}, ink);
}
int Menu::titleClick(sf::Vector2f p) const {
    if (p.x >= BUTTON_X && p.x <= BUTTON_X + BUTTON_W) {
        if (p.y >= 356.f && p.y <= 424.f) return 0;
        if (p.y >= 445.f && p.y <= 513.f) return 1;
        if (p.y >= 534.f && p.y <= 602.f) return 2;
    }
    return -1;
}
int Menu::stageClick(sf::Vector2f p, int count) const {
    for (int i = 0; i < count && i < 16; ++i) {
        const float x = 105.f + (i % 4) * 210.f;
        const float y = 190.f + (i / 4) * 103.f;
        if (p.x >= x && p.x <= x + 180.f && p.y >= y && p.y <= y + 86.f) return i;
    }
    return -1;
}

void Menu::drawEnding(sf::RenderWindow& w, EndingKind kind) const {
    const bool perfect = kind == EndingKind::Perfect;
    const bool trueEnding = kind == EndingKind::True;
    w.clear(perfect ? sf::Color(221, 198, 139) :
            trueEnding ? sf::Color(157, 204, 199) : sf::Color(179, 216, 198));
    block(w, {{110.f, 108.f}, {804.f, 472.f}}, cream);
    if (perfect) {
        text(w, "真・真・エンディング", 46, {512.f, 184.f}, ink);
        text(w, "全ステージ・全目標・最短手数達成！", 29, {512.f, 274.f}, ink);
        text(w, "すげーな、お前暇なのか？", 29, {512.f, 355.f}, ink);
        text(w, "……ここまで遊んでくれて、本当にありがとう！", 21, {512.f, 416.f}, ink);
    } else if (trueEnding) {
        text(w, "真・エンディング", 49, {512.f, 184.f}, ink);
        text(w, "全ステージ・両目標達成！", 33, {512.f, 280.f}, ink);
        text(w, "次は全目標の最短手数に挑戦してみよう！", 23, {512.f, 389.f}, ink);
    } else {
        text(w, "エンディング", 51, {512.f, 184.f}, ink);
        text(w, "全ステージクリア！", 40, {512.f, 280.f}, ink);
        text(w, "もう一方の目標達成もがんばれ！", 25, {512.f, 389.f}, ink);
    }
    text(w, "Enterキーでステージ選択へ", 22, {512.f, 505.f}, ink);
}
void Menu::drawPlayingResult(sf::RenderWindow& w, const State& s) const {
    if (s.result == Result::Playing) return;
    block(w, {{105.f, 278.f}, {610.f, 166.f}}, sf::Color(32, 40, 52));
    text(w, s.result == Result::Win ? "目標達成！" : "失敗！", 48,
         {410.f, 345.f}, s.result == Result::Win ? sf::Color(126, 234, 157) : sf::Color(255, 141, 143));
    text(w, "Q：ステージ選択へ　 R：やり直す", 19, {410.f, 402.f}, sf::Color::White);
}

void Menu::drawHowTo(sf::RenderWindow& w) const {
    w.clear(sf::Color(179, 216, 198));
    text(w, "ゲーム説明", 46, {512.f, 65.f}, ink);
    block(w, {{72.f, 115.f}, {880.f, 520.f}}, cream);
    auto section = [&](const std::string& title, const std::vector<std::string>& lines, float y) {
        text(w, title, 25, {110.f, y}, ink, false);
        for (std::size_t i = 0; i < lines.size(); ++i)
            text(w, lines[i], 19, {118.f, y + 40.f + 30.f * static_cast<float>(i)}, ink, false);
    };
    section("目標", {"泥棒を操作してアジトに到達するか、警察を操作して泥棒を捕まえれば成功です。",
                     "両方の目標達成を目指しましょう。"}, 133.f);
    section("2人の動きは連動する", {"泥棒を操作すると警察も同じ方向へ、警察を操作すると泥棒は反対方向へ動こうとします。",
                              "壁があれば移動できません。"}, 268.f);
    section("移動制約とギミック", {"操作中のキャラクターは後退できず、壁の方を向けません。",
                              "スイッチで操作対象が交代し、誰かがトゲに触れると失敗です。（キャラが〇んだら悲しいので）"}, 403.f);
    text(w, "WASD：移動　 Z：1手戻す　 R：やり直す　 Q：ステージ選択", 17,
         {512.f, 578.f}, ink);
    text(w, "Esc：タイトルへ戻る", 20, {512.f, 670.f}, ink);
}
