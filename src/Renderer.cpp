#include "Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <filesystem>

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

// Textures are loaded once, not on every frame.
struct CharacterTextures {
    sf::Texture hideout;
    sf::Texture spikes;
    sf::Texture sw;
    bool hideoutReady = false;
    bool spikesReady = false;
    bool switchReady = false;
    sf::Texture thief;
    sf::Texture police;
    bool thiefReady = false;
    bool policeReady = false;

    CharacterTextures() {
        hideoutReady = hideout.loadFromFile("assets/images/hideout.png");
        spikesReady = spikes.loadFromFile("assets/images/spikes.png");
        switchReady = sw.loadFromFile("assets/images/switch.png");
        thiefReady = thief.loadFromFile("assets/images/thief.jpg");
        policeReady = police.loadFromFile("assets/images/police.jpg");
        if (hideoutReady) hideout.setSmooth(true);
        if (spikesReady) spikes.setSmooth(true);
        if (switchReady) sw.setSmooth(true);
        if (thiefReady) thief.setSmooth(true);
        if (policeReady) police.setSmooth(true);
    }
};

void drawCharacter(sf::RenderWindow& window, const Character& c,
                   const sf::Texture* texture, sf::Color fallbackColor,
                   bool controlled) {
    const sf::Vector2f center(MARGIN + (c.x + .5f) * TILE,
                              MARGIN + (c.y + .5f) * TILE);
    constexpr float CHARACTER_SIZE = 57.f;

    // A square highlight fits the square characters better than a circle.
    if (controlled) {
        sf::RectangleShape highlight({CHARACTER_SIZE + 8.f, CHARACTER_SIZE + 8.f});
        highlight.setOrigin({(CHARACTER_SIZE + 8.f) / 2.f,
                             (CHARACTER_SIZE + 8.f) / 2.f});
        highlight.setPosition(center);
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color(255, 211, 65));
        highlight.setOutlineThickness(3.f);
        window.draw(highlight);
    }

    if (texture) {
        sf::Sprite sprite(*texture);
        const sf::Vector2u imageSize = texture->getSize();
        sprite.setOrigin({imageSize.x / 2.f, imageSize.y / 2.f});
        sprite.setPosition(center);
        sprite.setScale({CHARACTER_SIZE / imageSize.x,
                         CHARACTER_SIZE / imageSize.y});
        // UP=0, RIGHT=1, DOWN=2, LEFT=3.
        sprite.setRotation(sf::degrees(90.f * static_cast<float>(c.direction)));
        window.draw(sprite);
        return;
    }

    // Fallback if an image is missing: retain the original circle and arrow.
    sf::CircleShape body(20.f);
    body.setOrigin({20.f, 20.f});
    body.setPosition(center);
    body.setFillColor(fallbackColor);
    window.draw(body);
    sf::ConvexShape arrow(3);
    arrow.setPoint(0, {0.f, -15.f});
    arrow.setPoint(1, {-9.f, 8.f});
    arrow.setPoint(2, {9.f, 8.f});
    arrow.setPosition(center);
    arrow.setRotation(sf::degrees(90.f * static_cast<float>(c.direction)));
    arrow.setFillColor(sf::Color::White);
    window.draw(arrow);
}

void drawBoard(sf::RenderWindow& window, const Board& b, const State& s) {
    static CharacterTextures textures;
    auto drawTileImage = [&](const sf::Texture& texture, sf::Vector2f pos) {
        const sf::Vector2u sz = texture.getSize();
        if (!sz.x || !sz.y) return;
        sf::Sprite sprite(texture);
        sprite.setOrigin({sz.x / 2.f, sz.y / 2.f});
        sprite.setPosition(pos + sf::Vector2f(TILE / 2.f, TILE / 2.f));
        const float scale = (TILE - 15.f) / static_cast<float>(std::max(sz.x, sz.y));
        sprite.setScale({scale, scale});
        window.draw(sprite);
    };
    for (int y = 0; y < b.height; ++y) {
        for (int x = 0; x < b.width; ++x) {
            if (b.cell[y][x] == ' ') continue;
            sf::Vector2f pos(MARGIN + x * TILE, MARGIN + y * TILE);
            rect(window, pos, {TILE - 1.f, TILE - 1.f}, sf::Color(56, 61, 75));
            char tile = b.cell[y][x];
            if (tile == 'A' && textures.hideoutReady) {
                drawTileImage(textures.hideout, pos);
            } else if (tile == 'S' && textures.switchReady) {
                drawTileImage(textures.sw, pos);
            } else if (tile == 'X' && textures.spikesReady) {
                drawTileImage(textures.spikes, pos);
            } else if (tile == 'A') {
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
            if (b.cell[y][x] == ' ') continue;
            const float left = MARGIN + x * TILE;
            const float top = MARGIN + y * TILE;
            if (b.wall[y][x][UP])
                rect(window, {left - half, top - half},
                     {TILE + WALL_THICKNESS, WALL_THICKNESS}, wallColor);
            if (b.wall[y][x][LEFT])
                rect(window, {left - half, top - half},
                     {WALL_THICKNESS, TILE + WALL_THICKNESS}, wallColor);
            if ((x == b.width - 1 || b.cell[y][x + 1] == ' ') && b.wall[y][x][RIGHT])
                rect(window, {left + TILE - half, top - half},
                     {WALL_THICKNESS, TILE + WALL_THICKNESS}, wallColor);
            if ((y == b.height - 1 || b.cell[y + 1][x] == ' ') && b.wall[y][x][DOWN])
                rect(window, {left - half, top + TILE - half},
                     {TILE + WALL_THICKNESS, WALL_THICKNESS}, wallColor);
        }
    }
    drawCharacter(window, s.thief, textures.thiefReady ? &textures.thief : nullptr,
                  sf::Color(67, 154, 244), s.controlThief);
    drawCharacter(window, s.police, textures.policeReady ? &textures.police : nullptr,
                  sf::Color(240, 95, 99), !s.controlThief);
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
    std::string title = "Thief and Police | 操作中：";
    title += s.controlThief ? "泥棒" : "警察";
    if (s.result == Result::Win) title += " | 成功！";
    else if (s.result == Result::Lose) title += " | 失敗";
    if (!s.message.empty()) title += " | " + s.message;
    window.setTitle(sf::String::fromUtf8(title.begin(), title.end()));
}

// HUD: presentation only. All game rules remain in Game.cpp.
void drawHud(sf::RenderWindow& window, const Board& b, const State& s,
             std::size_t moves, const std::string& stageName) {
    const float x = MARGIN * 2.f + b.width * TILE + 16.f;
    const float top = 24.f;
    const float panelWidth = 268.f;
    const float panelHeight = 672.f;
    rect(window, {x, top}, {panelWidth, panelHeight}, sf::Color(39, 44, 58));
    rect(window, {x, top}, {5.f, panelHeight}, sf::Color(91, 169, 237));

    // Load a system font once. No font is distributed with the source code.
    static sf::Font font;
    static bool fontReady = [] {
        const char* paths[] = {
            "assets/fonts/menu.ttf",
            "assets/fonts/NotoSansCJKjp-Regular.otf",
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
            "/mnt/c/Windows/Fonts/meiryo.ttc",
            "/mnt/c/Windows/Fonts/YuGothR.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
        for (const char* path : paths)
            if (std::filesystem::exists(path) && font.openFromFile(path)) return true;
        return false;
    }();
    if (!fontReady) return;

    auto label = [&](const std::string& value, float y, unsigned size, sf::Color color) {
        sf::Text text(font, sf::String::fromUtf8(value.begin(), value.end()), size);
        text.setPosition({x + 20.f, y});
        text.setFillColor(color);
        window.draw(text);
    };
    const sf::Color muted(170, 181, 200);
    const sf::Color white(237, 242, 249);
    const sf::Color blue(86, 173, 255);
    const sf::Color red(250, 115, 120);
    label("Thief and Police", top + 18.f, 20, white);
    label("ステージ", top + 63.f, 12, muted);
    std::string shortName = stageName;
    const auto slash = shortName.find_last_of("/\\");
    if (slash != std::string::npos) shortName = shortName.substr(slash + 1);
    if (shortName.size() > 24) shortName = shortName.substr(0, 21) + "...";
    label(shortName, top + 82.f, 16, white);

    label("操作中", top + 125.f, 12, muted);
    rect(window, {x + 224.f, top + 149.f}, {14.f, 14.f}, s.controlThief ? blue : red);
    label(s.controlThief ? "泥棒" : "警察", top + 140.f, 23,
          s.controlThief ? blue : red);
    label("手数", top + 192.f, 12, muted);
    label(std::to_string(moves), top + 211.f, 26, white);

    label("操作方法", top + 265.f, 12, muted);
    label("WASD  移動", top + 286.f, 15, white);
    label("Z  1手戻す", top + 311.f, 15, white);
    label("R  やり直す", top + 336.f, 15, white);
    label("Q  ステージ選択", top + 361.f, 15, white);

    // Use the remaining space for status if the window is tall enough.
    if (panelHeight >= 470.f) {
        label("状態", top + 406.f, 12, muted);
        if (s.result == Result::Win) label("成功", top + 428.f, 20, sf::Color(65, 210, 130));
        else if (s.result == Result::Lose) label("失敗", top + 428.f, 20, red);
        else if (isStuck(b, s)) label("動けません", top + 428.f, 15, red);
        else label("プレイ中", top + 428.f, 17, white);
    }
}
