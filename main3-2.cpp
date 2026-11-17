#include "Game.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_set>
#include <algorithm>
using namespace std;
const string ARROWS[4] = {"^", ">", "v", "<"};
const string DIR_NAMES[4] = {"Up", "Right", "Down", "Left"};
string cellText(const Board& b, const State& s,
                int x, int y) {
    bool t = s.thief.x == x && s.thief.y == y;
    bool p = s.police.x == x && s.police.y == y;

    if (t && p) return "TP";
    if (t) return "T" + ARROWS[s.thief.direction];
    if (p) return "P" + ARROWS[s.police.direction];
    for (const Position& box : s.boxes)
        if (box.x == x && box.y == y) return "B ";

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

    if (s.result == Result::Win) {
        cout << "*** WIN ***\n";
    }
    else if (s.result == Result::Lose) {
        cout << "*** LOSE ***\n";
    }
}

// BFS distinguishes positions, facing directions, and control.
// Terminal states are not expanded.
string stateKey(const State& s) {
    string result = to_string(s.thief.x) + "," +
           to_string(s.thief.y) + "," +
           to_string(s.thief.direction) + "," +
           to_string(s.police.x) + "," +
           to_string(s.police.y) + "," +
           to_string(s.police.direction) + "," +
           (s.controlThief ? "T" : "P");
    vector<pair<int,int>> boxes;
    for (const Position& b : s.boxes) boxes.push_back({b.x,b.y});
    sort(boxes.begin(),boxes.end());
    for (auto [x,y] : boxes) result += ";" + to_string(x) + "," + to_string(y);
    return result;
}

struct SearchNode {
    State state;
    int parent;
    char move;
};

struct SearchResult {
    vector<SearchNode> nodes;
    int escapeNode = -1;
    int arrestNode = -1;
    size_t visitedCount = 0;
};

SearchResult solve(const Board& board, const State& initial) {
    SearchResult result;
    result.nodes.push_back({initial, -1, '-'});

    queue<int> pending;
    pending.push(0);

    unordered_set<string> visited;
    visited.insert(stateKey(initial));

    const int directions[4] = {UP, RIGHT, DOWN, LEFT};
    const char keys[4] = {'W', 'D', 'S', 'A'};

    while (!pending.empty()) {
        int currentIndex = pending.front();
        pending.pop();

        // Copy state before push_back may reallocate the vector.
        State current = result.nodes[currentIndex].state;

        for (int i = 0; i < 4; ++i) {
            State next = current;

            if (!takeTurn(board, next, directions[i])) {
                continue;
            }

            if (next.result == Result::Win) {
                // The winning side is the side controlled
                // before this move.
                bool escape = current.controlThief;

                int& found = escape
                    ? result.escapeNode
                    : result.arrestNode;

                if (found == -1) {
                    found = static_cast<int>(result.nodes.size());
                    result.nodes.push_back({
                        next, currentIndex, keys[i]
                    });
                }
                continue;
            }

            // Losing states cannot lead to a victory.
            if (next.result == Result::Lose) {
                continue;
            }

            string key = stateKey(next);

            if (!visited.insert(key).second) {
                continue;
            }

            int nextIndex =
                static_cast<int>(result.nodes.size());

            result.nodes.push_back({
                next, currentIndex, keys[i]
            });

            pending.push(nextIndex);
        }

        if (result.escapeNode != -1 &&
            result.arrestNode != -1) {
            break;
        }
    }

    result.visitedCount = visited.size();
    return result;
}

void printSolution(const SearchResult& result,
                   int goalIndex,
                   const string& title) {
    cout << "\n=== " << title << " ===\n";

    if (goalIndex == -1) {
        cout << "Impossible: no winning route exists.\n";
        return;
    }

    vector<int> route;

    for (int index = goalIndex;
         index != -1;
         index = result.nodes[index].parent) {
        route.push_back(index);
    }

    reverse(route.begin(), route.end());

    cout << "Minimum moves: "
         << route.size() - 1 << "\n";

    cout << "Input sequence: ";

    for (size_t i = 1; i < route.size(); ++i) {
        if (i > 1) cout << " -> ";
        cout << result.nodes[route[i]].move;
    }

    cout << "\n";

    cout << "\nStep-by-step states:\n";

    for (size_t i = 0; i < route.size(); ++i) {
        const State& s = result.nodes[route[i]].state;

        cout << "Step " << i;

        if (i > 0) {
            cout << " ["
                 << result.nodes[route[i]].move
                 << "]";
        }

        cout << ": T("
             << s.thief.x << ","
             << s.thief.y << ")"
             << ARROWS[s.thief.direction]

             << "  P("
             << s.police.x << ","
             << s.police.y << ")"
             << ARROWS[s.police.direction]

             << "  Control="
             << (s.controlThief ? "Thief" : "Police");

        if (s.result == Result::Win) {
            cout << "  WIN";
        }

        cout << "  Boxes=" << s.boxes.size();
        if (!s.crashes.empty()) cout << "  CRASH=" << s.crashes.size();
        if (!s.pushes.empty()) cout << "  PUSH=" << s.pushes.size();
        cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    string filename = "stages/stage01.txt";

    if (argc >= 2) {
        filename = argv[1];
    }

    Board board;
    State initial{};

    if (!loadStage(filename, board, initial)) {
        return 1;
    }

    cout << "Thief & Police Puzzle - BFS Solver\n";
    cout << "Stage: " << filename << "\n";

    cout << "Size (rows x columns): "
         << board.height << "x"
         << board.width << "\n";

    printBoard(board, initial);

    SearchResult result = solve(board, initial);

    printSolution(
        result,
        result.escapeNode,
        "Thief escape to hideout"
    );

    printSolution(
        result,
        result.arrestNode,
        "Police arrest"
    );

    cout << "\nVisited non-terminal states: "
         << result.visitedCount << "\n";

    return 0;
}
