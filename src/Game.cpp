#include "Game.hpp"
#include <iostream>
#include <fstream>

using namespace std;
const int dx[4] = {0, 1, 0, -1};
const int dy[4] = {-1, 0, 1, 0};

int opposite(int d) {
    return (d + 2) % 4;
}

bool isInside(const Board& b, int x, int y) {
    return x >= 0 && x < b.width &&
           y >= 0 && y < b.height &&
           b.cell[y][x] != ' ';
}

int arrowDirection(char c) {
    if (c == '^') return UP;
    if (c == '>') return RIGHT;
    if (c == 'v') return DOWN;
    if (c == '<') return LEFT;
    return -1;
}

bool canMove(const Board& b, const Character& c, int d) {
    int nx = c.x + dx[d];
    int ny = c.y + dy[d];

    return isInside(b, nx, ny) &&
           !b.wall[c.y][c.x][d];
}

bool isStuck(const Board& b, const State& s) {
    const Character& c =
        s.controlThief ? s.thief : s.police;

    for (int d = 0; d < 4; ++d) {
        if (d != opposite(c.direction) &&
            canMove(b, c, d)) {
            return false;
        }
    }
    return true;
}

// Read a stage from an ASCII text file.
// First line: rows columns initialControl
bool loadStage(const string& filename, Board& b, State& s) {
    ifstream file(filename);

    if (!file) {
        cerr << "Cannot open file: " << filename << "\n";
        return false;
    }

    int h, w;
    char initialControl;

    if (!(file >> h >> w >> initialControl) ||
        (initialControl != 'T' && initialControl != 'P') ||
        w <= 0 || h <= 0 ||
        w > 100 || h > 100) {
        cerr << "Invalid stage dimensions or initial control.\n";
        return false;
    }

    string line;
    getline(file, line);

    vector<string> lines;

    for (int i = 0; i < 2 * h + 1; ++i) {
        if (!getline(file, line)) {
            cerr << "Stage is incomplete.\n";
            return false;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (static_cast<int>(line.size()) != 4 * w + 1) {
            cerr << "Incorrect line width at board line "
                 << i + 1 << ". Expected " << 4 * w + 1
                 << " characters, got " << line.size() << ".\n";
            return false;
        }

        lines.push_back(line);
    }

    b.width = w;
    b.height = h;
    b.cell.assign(h, vector<char>(w, ' '));
    b.wall.assign(
        h, vector<array<bool, 4>>(w, {false, false, false, false})
    );

    int thiefCount = 0;
    int policeCount = 0;

    // Parse horizontal walls and corners.
    for (int y = 0; y <= h; ++y) {
        const string& row = lines[2 * y];

        for (int x = 0; x <= w; ++x) {
            if (row[4 * x] != '+' && row[4 * x] != ' ') {
                cerr << "Invalid corner in stage.\n";
                return false;
            }
        }

        for (int x = 0; x < w; ++x) {
            string segment = row.substr(4 * x + 1, 3);

            if (segment != "---" && segment != "   ") {
                cerr << "Invalid horizontal wall.\n";
                return false;
            }

            bool hasWall = segment == "---";

            if (y > 0) b.wall[y - 1][x][DOWN] = hasWall;
            if (y < h) b.wall[y][x][UP] = hasWall;
        }
    }

    // Parse vertical walls and cell contents.
    for (int y = 0; y < h; ++y) {
        const string& row = lines[2 * y + 1];

        for (int x = 0; x <= w; ++x) {
            char separator = row[4 * x];

            if (separator != '|' && separator != ' ') {
                cerr << "Invalid vertical wall.\n";
                return false;
            }

            bool hasWall = separator == '|';

            if (x > 0) b.wall[y][x - 1][RIGHT] = hasWall;
            if (x < w) b.wall[y][x][LEFT] = hasWall;
        }

        for (int x = 0; x < w; ++x) {
            string token = row.substr(4 * x + 1, 2);

            if (token[0] == 'T' || token[0] == 'P') {
                b.cell[y][x] = '.';
                int d = arrowDirection(token[1]);

                if (d == -1) {
                    cerr << "Invalid character direction.\n";
                    return false;
                }

                if (token[0] == 'T') {
                    s.thief = {x, y, d};
                    ++thiefCount;
                } else {
                    s.police = {x, y, d};
                    ++policeCount;
                }
            }
            else if (token == "  " ) {
                b.cell[y][x] = ' ';
            }
            else if (token[1] == ' ' &&
                     (token[0] == '.' ||
                      token[0] == 'A' ||
                      token[0] == 'S' ||
                      token[0] == 'X')) {
                b.cell[y][x] = token[0];
            }
            else {
                cerr << "Invalid cell at ("
                     << x << "," << y << "): ["
                     << token << "]\n";
                return false;
            }

            if (row[4 * x + 3] != ' ') {
                cerr << "Missing cell padding space.\n";
                return false;
            }
        }
    }

    if (thiefCount != 1 || policeCount != 1) {
        cerr << "Stage must contain exactly one T and one P.\n";
        return false;
    }

    // Every playable cell must be separated from absent cells and the
    // outside of the bounding rectangle by a wall.
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (b.cell[y][x] == ' ') continue;
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if ((nx < 0 || nx >= w || ny < 0 || ny >= h ||
                     b.cell[ny][nx] == ' ') && !b.wall[y][x][d]) {
                    cerr << "Missing boundary wall at (" << x << "," << y
                         << ") direction " << d << "\n";
                    return false;
                }
            }
        }
    }

    s.controlThief = (initialControl == 'T');
    s.result = Result::Playing;
    s.message.clear();

    return true;
}

// Advance the game by one valid move.
bool takeTurn(const Board& b, State& s, int d) {
    if (s.result != Result::Playing) {
        s.message = "終了しました。Z：戻す / R：やり直す";
        return false;
    }

    const Character& controlled =
        s.controlThief ? s.thief : s.police;

    // 1. No backward movement for the controlled side.
    if (d == opposite(controlled.direction)) {
        s.message = "後ろには進めません！";
        return false;
    }

    // 2. Invalid controlled movement cancels the turn.
    if (!canMove(b, controlled, d)) {
        s.message = "壁があって進めません！";
        return false;
    }

    State next = s;
    next.message.clear();

    Character& moving =
        s.controlThief ? next.thief : next.police;

    Character& other =
        s.controlThief ? next.police : next.thief;

    int oldTx = s.thief.x, oldTy = s.thief.y;
    int oldPx = s.police.x, oldPy = s.police.y;

    moving.x += dx[d];
    moving.y += dy[d];
    moving.direction = d;

    int otherDir = s.controlThief ? d : opposite(d);

    if (canMove(b, other, otherDir)) {
        other.x += dx[otherDir];
        other.y += dy[otherDir];
    }

    // Facing changes even when the other character is blocked.
    other.direction = otherDir;

    // 3. Spikes take priority over every other result.
    bool onSpike =
        b.cell[next.thief.y][next.thief.x] == 'X' ||
        b.cell[next.police.y][next.police.x] == 'X';

    if (onSpike) {
        next.result = Result::Lose;
        next.message = "トゲに触れてしまいました！";
    }
    else {
        // 4. Meeting or crossing.
        bool sameCell =
            next.thief.x == next.police.x &&
            next.thief.y == next.police.y;

        bool crossed =
            next.thief.x == oldPx &&
            next.thief.y == oldPy &&
            next.police.x == oldTx &&
            next.police.y == oldTy;

        if (sameCell || crossed) {
            next.result =
                s.controlThief ? Result::Lose : Result::Win;

            next.message = s.controlThief
                ? "泥棒が逮捕されました！"
                : "警察が泥棒を逮捕しました！";
        }
        else if (b.cell[next.thief.y][next.thief.x] == 'A') {
            // 5. Thief reaches hideout.
            next.result =
                s.controlThief ? Result::Win : Result::Lose;

            next.message = s.controlThief
                ? "泥棒がアジトに到着しました！"
                : "泥棒が逃げ切りました！";
        }
    }

    // 6. Only the controlled character activates a switch
    // by entering its cell. Terminal results take priority.
    if (next.result == Result::Playing) {
        const Character& c =
            s.controlThief ? next.thief : next.police;

        if (b.cell[c.y][c.x] == 'S') {
            next.controlThief = !s.controlThief;
            next.message = "スイッチで操作が交代しました！";
        }
    }

    s = next;
    return true;
}

