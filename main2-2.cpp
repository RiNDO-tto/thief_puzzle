#include "Game.hpp"
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
static const char* ARROWS[] = {"^", ">", "v", "<"};
static const char* DIR_NAMES[] = {"Up", "Right", "Down", "Left"};

string cellText(const Board& b, const State& s, int x, int y) {
    if (s.thief.x == x && s.thief.y == y && s.police.x == x && s.police.y == y) return "TP";
    if (s.thief.x == x && s.thief.y == y) return string("T") + ARROWS[s.thief.direction];
    if (s.police.x == x && s.police.y == y) return string("P") + ARROWS[s.police.direction];
    if (b.cell[y][x] == ' ') return "  ";
    return string(1, b.cell[y][x]) + " ";
}

void printBoard(const Board& b, const State& s) {
    cout << '\n';
    for (int y = 0; y < b.height; ++y) {
        for (int x = 0; x < b.width; ++x) {
            bool corner = b.cell[y][x] != ' ' ||
                (y > 0 && b.cell[y-1][x] != ' ') ||
                (x > 0 && b.cell[y][x-1] != ' ') ||
                (x > 0 && y > 0 && b.cell[y-1][x-1] != ' ');
            cout << (corner ? '+' : ' ');
            bool wall = b.wall[y][x][UP];
            cout << (wall ? "---" : "   ");
        }
        bool endCorner = b.cell[y][b.width-1] != ' ' ||
            (y > 0 && b.cell[y-1][b.width-1] != ' ');
        cout << (endCorner ? '+' : ' ') << '\n';
        for (int x = 0; x < b.width; ++x) {
            cout << (b.wall[y][x][LEFT] ? '|' : ' ');
            cout << cellText(b, s, x, y) << ' ';
        }
        cout << (b.wall[y][b.width-1][RIGHT] ? '|' : ' ') << '\n';
    }
    for (int x = 0; x < b.width; ++x) {
        bool corner = b.cell[b.height-1][x] != ' ' ||
            (x > 0 && b.cell[b.height-1][x-1] != ' ');
        cout << (corner ? '+' : ' ')
             << (b.wall[b.height-1][x][DOWN] ? "---" : "   ");
    }
    cout << (b.cell[b.height-1][b.width-1] != ' ' ? '+' : ' ') << '\n';
    const Character& c = s.controlThief ? s.thief : s.police;
    cout << "Control: " << (s.controlThief ? "Thief" : "Police")
         << " | Facing: " << DIR_NAMES[c.direction] << ' ' << ARROWS[c.direction] << '\n';
    cout << "Thief: (" << s.thief.x << ',' << s.thief.y << ") " << ARROWS[s.thief.direction] << '\n';
    cout << "Police: (" << s.police.x << ',' << s.police.y << ") " << ARROWS[s.police.direction] << '\n';
    if (!s.message.empty()) cout << s.message << '\n';
    if (s.result == Result::Playing && isStuck(b,s)) cout << "No legal moves remain! Press Z to undo or R to restart.\n";
    if (s.result == Result::Win) cout << "*** WIN ***\n";
    else if (s.result == Result::Lose) cout << "*** LOSE ***\n";
}

int main(int argc, char* argv[]) {
    const string filename = argc >= 2 ? argv[1] : "stages/stage01.txt";
    Board board;
    State state{};
    if (!loadStage(filename, board, state)) return 1;
    const State initialState = state;
    vector<State> history;
    cout << "Thief & Police Puzzle\nStage: " << filename << '\n'
         << "Size (rows x columns): " << board.height << 'x' << board.width << '\n'
         << "W/A/S/D: Move | Z: Undo | R: Restart | Q: Quit\n";
    while (true) {
        printBoard(board, state);
        cout << "\nInput: " << flush;
        char input;
        if (!(cin >> input)) break;
        input = static_cast<char>(tolower(static_cast<unsigned char>(input)));
        if (input == 'q') break;
        if (input == 'r') { state = initialState; history.clear(); state.message = "Stage restarted!"; continue; }
        if (input == 'z') {
            if (history.empty()) state.message = "Nothing to undo!";
            else { state = history.back(); history.pop_back(); state.message = "Undid one move."; }
            continue;
        }
        int d = input == 'w' ? UP : input == 'd' ? RIGHT : input == 's' ? DOWN : input == 'a' ? LEFT : -1;
        if (d == -1) { state.message = "Invalid input!"; continue; }
        State previous = state;
        if (takeTurn(board, state, d)) history.push_back(previous);
    }
    return 0;
}
