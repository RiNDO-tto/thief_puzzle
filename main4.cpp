#include "src/Game.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <array>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <limits>


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

    if (s.result == Result::Playing && isStuck(b, s)) {
        cout << "No legal moves remain! Press R to undo.\n";
    }

    if (s.result == Result::Win) {
        cout << "*** WIN ***\n";
    }
    else if (s.result == Result::Lose) {
        cout << "*** LOSE ***\n";
    }
}

// BFS distinguishes positions, facing directions, and the controlled side.
// Result/message are not part of the key: terminal states are never expanded.
string stateKey(const State& s) {
    string result = to_string(s.thief.x) + "," + to_string(s.thief.y) + "," +
           to_string(s.thief.direction) + "," +
           to_string(s.police.x) + "," + to_string(s.police.y) + "," +
           to_string(s.police.direction) + "," +
           (s.controlThief ? "T" : "P");
    vector<pair<int,int>> boxes;
    for (const Position& b : s.boxes) boxes.emplace_back(b.x, b.y);
    sort(boxes.begin(), boxes.end());
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

        // Copy the state: push_back below can reallocate result.nodes.
        State current = result.nodes[currentIndex].state;

        for (int i = 0; i < 4; ++i) {
            State next = current;
            if (!takeTurn(board, next, directions[i])) continue;

            if (next.result == Result::Win) {
                // Winning side is determined BEFORE any switch in this turn.
                bool escape = current.controlThief;
                int& found = escape ? result.escapeNode : result.arrestNode;
                if (found == -1) {
                    found = static_cast<int>(result.nodes.size());
                    result.nodes.push_back({next, currentIndex, keys[i]});
                }
                continue;
            }

            // Losing states cannot lead to a winning state.
            if (next.result == Result::Lose) continue;

            string key = stateKey(next);
            if (!visited.insert(key).second) continue;

            int nextIndex = static_cast<int>(result.nodes.size());
            result.nodes.push_back({next, currentIndex, keys[i]});
            pending.push(nextIndex);
        }

        if (result.escapeNode != -1 && result.arrestNode != -1) break;
    }

    result.visitedCount = visited.size();
    return result;
}

void printSolution(const SearchResult& result, int goalIndex,
                   const string& title) {
    cout << "\n=== " << title << " ===\n";
    if (goalIndex == -1) {
        cout << "Impossible: no winning route exists.\n";
        return;
    }

    vector<int> route;
    for (int index = goalIndex; index != -1;
         index = result.nodes[index].parent) {
        route.push_back(index);
    }
    reverse(route.begin(), route.end());

    cout << "Minimum moves: " << route.size() - 1 << "\n";
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
        if (i > 0) cout << " [" << result.nodes[route[i]].move << "]";
        cout << ": T(" << s.thief.x << "," << s.thief.y << ")"
             << ARROWS[s.thief.direction]
             << "  P(" << s.police.x << "," << s.police.y << ")"
             << ARROWS[s.police.direction]
             << "  Control=" << (s.controlThief ? "Thief" : "Police");
        cout << "  Boxes=" << s.boxes.size();
        if (s.result == Result::Win) cout << "  WIN";
        cout << "\n";
    }
}


// A stage's playable shape is the set of cells reachable from the thief
// through open passages in the ORIGINAL board.
using Mask = vector<vector<bool>>;
Mask reachable(const Board& b, int sx, int sy) {
    Mask seen(b.height, vector<bool>(b.width, false));
    queue<pair<int,int>> q;
    seen[sy][sx] = true;
    q.push({sx,sy});
    while (!q.empty()) {
        auto [x,y] = q.front(); q.pop();
        for (int d=0; d<4; ++d) {
            int nx=x+dx[d], ny=y+dy[d];
            if (!isInside(b,nx,ny) || b.wall[y][x][d] || seen[ny][nx]) continue;
            seen[ny][nx]=true;
            q.push({nx,ny});
        }
    }
    return seen;
}

struct Edge { int x,y,d; }; // d=RIGHT or DOWN; each edge listed once
struct Edit { int kind, index, value; }; // 0: wall toggle, 1: thief facing, 2: police facing, 3: control

void toggleWall(Board& b, const Edge& e) {
    int nx=e.x+dx[e.d], ny=e.y+dy[e.d];
    bool value=!b.wall[e.y][e.x][e.d];
    b.wall[e.y][e.x][e.d]=value;
    b.wall[ny][nx][opposite(e.d)]=value;
}
void applyEdit(Board& b, State& s, const vector<Edge>& edges, const Edit& e) {
    if (e.kind==0) toggleWall(b,edges[e.index]);
    else if (e.kind==1) s.thief.direction=e.value;
    else if (e.kind==2) s.police.direction=e.value;
    else s.controlThief=!s.controlThief;
}

int pathLength(const SearchResult& result, int index) {
    if (index<0) return -1;
    int n=0;
    while (result.nodes[index].parent!=-1) {
        ++n;
        index=result.nodes[index].parent;
    }
    return n;
}
void report(const SearchResult& r) {
    int t=pathLength(r,r.escapeNode), p=pathLength(r,r.arrestNode);
    cout << "  Thief escape : ";
    if (t<0) cout << "Impossible\n"; else cout << t << " moves\n";
    cout << "  Police arrest: ";
    if (p<0) cout << "Impossible\n"; else cout << p << " moves\n";
}

bool saveStage(const string& filename, const Board& b, const State& s) {
    ofstream out(filename);
    if (!out) return false;
    out << b.height << ' ' << b.width << ' ' << (s.controlThief?'T':'P') << '\n';
    for (int y=0;y<b.height;++y) {
        for (int x=0;x<b.width;++x)
            out << '+' << (b.wall[y][x][UP]?"---":"   ");
        out << "+\n";
        for (int x=0;x<b.width;++x) {
            out << (b.wall[y][x][LEFT]?'|':' ');
            if (s.thief.x==x && s.thief.y==y)
                out << 'T' << ARROWS[s.thief.direction] << ' ';
            else if (s.police.x==x && s.police.y==y)
                out << 'P' << ARROWS[s.police.direction] << ' ';
            else if (any_of(s.boxes.begin(), s.boxes.end(),
                            [&](const Position& box) { return box.x == x && box.y == y; }))
                out << "B  ";
            else if (b.cell[y][x] == ' ') out << "   ";
            else out << b.cell[y][x] << "  ";
        }
        out << (b.wall[y][b.width-1][RIGHT]?'|':' ') << '\n';
    }
    for (int x=0;x<b.width;++x)
        out << '+' << (b.wall[b.height-1][x][DOWN]?"---":"   ");
    out << "+\n";
    return static_cast<bool>(out);
}

string editName(const Edit& e, const vector<Edge>& edges) {
    if (e.kind==0) {
        const Edge& a=edges[e.index];
        return "Toggle internal wall between (" + to_string(a.y+1) + "," +
            to_string(a.x+1) + ") and (" + to_string(a.y+dy[a.d]+1) +
            "," + to_string(a.x+dx[a.d]+1) + ")";
    }
    if (e.kind==1) return "Thief facing -> " + DIR_NAMES[e.value];
    if (e.kind==2) return "Police facing -> " + DIR_NAMES[e.value];
    return "Switch initial control";
}

int main(int argc, char* argv[]) {
    if (argc<2 || argc>5) {
        cerr << "Usage: " << argv[0]
             << " INPUT_STAGE [OUTPUT_STAGE] [MAX_EDITS=3] [MAX_CANDIDATES=200000]\n";
        return 1;
    }
    string input=argv[1];
    string output=argc>=3?argv[2]:input+".fixed.txt";
    if (input==output) {
        cerr << "Output must differ from input to protect the original stage.\n";
        return 1;
    }
    int maxEdits=3;
    size_t maxCandidates=200000;
    try {
        if (argc>=4) maxEdits=stoi(argv[3]);
        if (argc>=5) maxCandidates=stoull(argv[4]);
    } catch (const exception&) {
        cerr << "Invalid numeric argument.\n"; return 1;
    }
    if (maxEdits<0 || maxEdits>20 || maxCandidates==0) {
        cerr << "MAX_EDITS must be 0..20; MAX_CANDIDATES must be positive.\n";
        return 1;
    }
    Board original;
    State initial{};
    if (!loadStage(input,original,initial)) return 1;
    Mask shape=reachable(original,initial.thief.x,initial.thief.y);
    if (!shape[initial.police.y][initial.police.x]) {
        cerr << "Invalid stage: T and P are not connected.\n"; return 1;
    }
    int hideouts=0;
    for (int y=0;y<original.height;++y)
        for (int x=0;x<original.width;++x)
            if (original.cell[y][x]=='A') {
                ++hideouts;
                if (!shape[y][x]) {
                    cerr << "Invalid stage: hideout is outside playable shape.\n";
                    return 1;
                }
            }
    if (hideouts==0) {
        cerr << "Invalid stage: no hideout A.\n"; return 1;
    }

    cout << "Original stage: " << input << '\n';
    SearchResult originalResult=solve(original,initial);
    report(originalResult);
    if (originalResult.escapeNode>=0 && originalResult.arrestNode>=0) {
        cout << "Both goals are achievable. No modification needed.\n";
        return 0;
    }

    vector<Edge> edges;
    for (int y=0;y<original.height;++y) for (int x=0;x<original.width;++x) {
        if (!shape[y][x]) continue;
        for (int d : {RIGHT,DOWN}) {
            int nx=x+dx[d], ny=y+dy[d];
            if (isInside(original,nx,ny) && shape[ny][nx])
                edges.push_back({x,y,d});
        }
    }
    // Groups of edits: each physical wall can be toggled once;
    // each facing/control field can be assigned once.
    vector<vector<Edit>> groups;
    for (int i=0;i<(int)edges.size();++i) groups.push_back({{0,i,0}});
    vector<Edit> thiefOptions,policeOptions;
    for (int d=0;d<4;++d) {
        if (d!=initial.thief.direction) thiefOptions.push_back({1,0,d});
        if (d!=initial.police.direction) policeOptions.push_back({2,0,d});
    }
    groups.push_back(thiefOptions);
    groups.push_back(policeOptions);
    groups.push_back({{3,0,0}});

    size_t tested=0, shapeRejected=0;
    bool found=false, limitReached=false;
    Board winnerBoard;
    State winnerState{};
    SearchResult winnerResult;
    vector<Edit> chosen,winnerEdits;

    // Enumerate exactly k distinct modified fields, in increasing k.
    // This guarantees minimum number of edits if the search completes a layer.
    function<void(int,int,Board&,State&)> dfs;
    dfs=[&](int start,int remaining,Board& b,State& s) {
        if (found || limitReached) return;
        if (remaining==0) {
            if (reachable(b,s.thief.x,s.thief.y)!=shape) {
                ++shapeRejected; return;
            }
            if (tested>=maxCandidates) { limitReached=true; return; }
            ++tested;
            SearchResult r=solve(b,s);
            if (r.escapeNode>=0 && r.arrestNode>=0) {
                found=true;
                winnerBoard=b;
                winnerState=s;
                winnerResult=std::move(r);
                winnerEdits=chosen;
            }
            return;
        }
        if ((int)groups.size()-start<remaining) return;
        for (int g=start;g<=(int)groups.size()-remaining && !found && !limitReached;++g) {
            for (const Edit& e:groups[g]) {
                applyEdit(b,s,edges,e);
                chosen.push_back(e);
                dfs(g+1,remaining-1,b,s);
                chosen.pop_back();
                // Restore exact original value for this field.
                if (e.kind==0) applyEdit(b,s,edges,e);
                else if (e.kind==1) s.thief.direction=initial.thief.direction;
                else if (e.kind==2) s.police.direction=initial.police.direction;
                else s.controlThief=initial.controlThief;
                if (found || limitReached) break;
            }
        }
    };

    cout << "\nSearching for minimum edits (playable shape fixed)...\n";
    for (int k=1;k<=maxEdits && !found && !limitReached;++k) {
        cout << "  Trying " << k << " edit(s)..." << flush;
        Board candidate=original;
        State candidateState=initial;
        size_t before=tested;
        dfs(0,k,candidate,candidateState);
        cout << " " << tested-before << " candidates evaluated\n";
    }
    cout << "BFS evaluations: " << tested << " (shape mismatches skipped: "
         << shapeRejected << ")\n";
    if (!found) {
        if (limitReached) cout << "Stopped: candidate limit reached.\n";
        else cout << "No solution within the specified edit limit.\n";
        return 2;
    }
    cout << "\nModified stage (" << winnerEdits.size() << " edit(s)):\n";
    for (const Edit& e:winnerEdits) cout << "  - " << editName(e,edges) << '\n';
    report(winnerResult);
    if (!saveStage(output,winnerBoard,winnerState)) {
        cerr << "Failed to write: " << output << '\n'; return 1;
    }
    cout << "Saved: " << output << '\n';
    return 0;
}
