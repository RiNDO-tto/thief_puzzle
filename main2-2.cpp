
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <array>

using namespace std;

enum Direction {
    UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3
};

const int dx[4] = {0, 1, 0, -1};
const int dy[4] = {-1, 0, 1, 0};
const string ARROWS[4] = {"^", ">", "v", "<"};
const string DIR_NAMES[4] = {"Up", "Right", "Down", "Left"};

enum class Result {
    Playing, Win, Lose
};

struct Character {
    int x, y, direction;
};

struct State {
    Character thief;
    Character police;
    bool controlThief = true;
    Result result = Result::Playing;
    string message;
};

struct Board {
    int width = 0;
    int height = 0;
    vector<vector<char>> cell;
    vector<vector<array<bool, 4>>> wall;
};

int opposite(int d) {
    return (d + 2) % 4;
}

bool isInside(const Board& b, int x, int y) {
    return x >= 0 && x < b.width &&
           y >= 0 && y < b.height;
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
    b.cell.assign(h, vector<char>(w, '.'));
    b.wall.assign(
        h, vector<array<bool, 4>>(w, {false, false, false, false})
    );

    int thiefCount = 0;
    int policeCount = 0;

    // Parse horizontal walls and corners.
    for (int y = 0; y <= h; ++y) {
        const string& row = lines[2 * y];

        for (int x = 0; x <= w; ++x) {
            if (row[4 * x] != '+') {
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

    // Require walls around the outside of the stage.
    for (int x = 0; x < w; ++x) {
        if (!b.wall[0][x][UP] ||
            !b.wall[h - 1][x][DOWN]) {
            cerr << "Outer horizontal walls are missing.\n";
            return false;
        }
    }

    for (int y = 0; y < h; ++y) {
        if (!b.wall[y][0][LEFT] ||
            !b.wall[y][w - 1][RIGHT]) {
            cerr << "Outer vertical walls are missing.\n";
            return false;
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
        s.message = "Game over. Press Z to undo or R to restart.";
        return false;
    }

    const Character& controlled =
        s.controlThief ? s.thief : s.police;

    // 1. No backward movement for the controlled side.
    if (d == opposite(controlled.direction)) {
        s.message = "Cannot move backward!";
        return false;
    }

    // 2. Invalid controlled movement cancels the turn.
    if (!canMove(b, controlled, d)) {
        s.message = "Blocked by a wall or boundary!";
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
        next.message = "Someone touched spikes!";
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
                ? "The thief was arrested!"
                : "The police arrested the thief!";
        }
        else if (b.cell[next.thief.y][next.thief.x] == 'A') {
            // 5. Thief reaches hideout.
            next.result =
                s.controlThief ? Result::Win : Result::Lose;

            next.message = s.controlThief
                ? "The thief reached the hideout!"
                : "The thief escaped!";
        }
    }

    // 6. Only the controlled character activates a switch
    // by entering its cell. Terminal results take priority.
    if (next.result == Result::Playing) {
        const Character& c =
            s.controlThief ? next.thief : next.police;

        if (b.cell[c.y][c.x] == 'S') {
            next.controlThief = !s.controlThief;
            next.message = "Switch activated! Control changed.";
        }
    }

    s = next;
    return true;
}

string cellText(const Board& b, const State& s,
                int x, int y) {
    bool t = s.thief.x == x && s.thief.y == y;
    bool p = s.police.x == x && s.police.y == y;

    if (t && p) return "TP";
    if (t) return "T" + ARROWS[s.thief.direction];
    if (p) return "P" + ARROWS[s.police.direction];

    return string(1, b.cell[y][x]) + " ";
}

void printBoard(const Board& b, const State& s) {
    cout << "\n";

    for (int y = 0; y < b.height; ++y) {
        for (int x = 0; x < b.width; ++x) {
            cout << "+"
                 << (b.wall[y][x][UP] ? "---" : "   ");
        }
        cout << "+\n";

        for (int x = 0; x < b.width; ++x) {
            cout << (b.wall[y][x][LEFT] ? "|" : " ");
            cout << cellText(b, s, x, y) << " ";
        }

        cout << (b.wall[y][b.width - 1][RIGHT] ? "|" : " ")
             << "\n";
    }

    for (int x = 0; x < b.width; ++x) {
        cout << "+"
             << (b.wall[b.height - 1][x][DOWN] ? "---" : "   ");
    }
    cout << "+\n";

    const Character& c =
        s.controlThief ? s.thief : s.police;

    cout << "Control: "
         << (s.controlThief ? "Thief" : "Police")
         << " | Facing: "
         << DIR_NAMES[c.direction]
         << " " << ARROWS[c.direction] << "\n";

    cout << "Thief: (" << s.thief.x << ","
         << s.thief.y << ") "
         << ARROWS[s.thief.direction] << "\n";

    cout << "Police: (" << s.police.x << ","
         << s.police.y << ") "
         << ARROWS[s.police.direction] << "\n";

    if (!s.message.empty()) {
        cout << s.message << "\n";
    }

    if (s.result == Result::Playing && isStuck(b, s)) {
        cout << "No legal moves remain! Press Z to undo or R to restart.\n";
    }

    if (s.result == Result::Win) {
        cout << "*** WIN ***\n";
    }
    else if (s.result == Result::Lose) {
        cout << "*** LOSE ***\n";
    }
}

int main(int argc, char* argv[]) {
    string filename = "stages/stage01.txt";

    if (argc >= 2) {
        filename = argv[1];
    }

    Board board;
    State state{};

    if (!loadStage(filename, board, state)) {
        return 1;
    }

    const State initialState = state;
    vector<State> history;

    cout << "Thief & Police Puzzle\n";
    cout << "Stage: " << filename << "\n";
    cout << "Size (rows x columns): " << board.height << "x"
         << board.width << "\n";
    cout << "W/A/S/D: Move | Z: Undo | R: Restart | Q: Quit\n";

    while (true) {
        printBoard(board, state);

        cout << "\nInput: ";

        char input;
        if (!(cin >> input)) break;

        input = static_cast<char>(
            tolower(static_cast<unsigned char>(input))
        );

        if (input == 'q') break;

        if (input == 'r') {
            state = initialState;
            history.clear();
            state.message = "Stage restarted!";
            continue;
        }

        if (input == 'z') {
            if (history.empty()) {
                state.message = "Nothing to undo!";
            }
            else {
                state = history.back();
                history.pop_back();
                state.message = "Undid one move.";
            }
            continue;
        }

        int d = -1;

        if (input == 'w') d = UP;
        else if (input == 'd') d = RIGHT;
        else if (input == 's') d = DOWN;
        else if (input == 'a') d = LEFT;

        if (d == -1) {
            state.message = "Invalid input!";
            continue;
        }

        State previous = state;

        if (takeTurn(board, state, d)) {
            history.push_back(previous);
        }
    }

    return 0;
}
