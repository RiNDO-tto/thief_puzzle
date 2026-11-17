#include "Game.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>

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
            ([&]() { State probe = s; return takeTurn(b, probe, d); })()) {
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

    s.boxes.clear();
    s.crashes.clear();
    s.pushes.clear();
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
                      token[0] == 'X' ||
                       token[0] == 'B')) {
                if (token[0] == 'B') {
                    b.cell[y][x] = '.';
                    s.boxes.push_back({x, y});
                } else b.cell[y][x] = token[0];
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

// All proposed positions are computed from the OLD state. Never mutate it during resolution.
namespace {
using Cell = std::pair<int,int>;
Cell cell(int x, int y) { return {x,y}; }
Cell nextCell(Cell c, int d) { return {c.first + dx[d], c.second + dy[d]}; }
bool edgeOpen(const Board& b, Cell c, int d) {
    Cell n = nextCell(c,d);
    return isInside(b,n.first,n.second) && !b.wall[c.second][c.first][d];
}
struct Intent {
    Cell origin, destination;
    int direction;
    bool moving = false;
    bool pushes = false;
    std::vector<Cell> chain;
};
Intent plan(const Board& b, Cell origin, int dir, const std::set<Cell>& boxes) {
    Intent r;
    r.origin = origin; r.destination = origin; r.direction = dir;
    Cell cursor = origin;
    while (edgeOpen(b,cursor,dir)) {
        cursor = nextCell(cursor,dir);
        if (!boxes.count(cursor)) {
            r.moving = true;
            r.destination = nextCell(origin,dir);
            r.pushes = !r.chain.empty();
            return r;
        }
        r.chain.push_back(cursor);
    }
    return r; // wall or board boundary prevents entire chain
}
}

bool takeTurn(const Board& b, State& s, int d) {
    if (s.result != Result::Playing) {
        s.message = "終了しました。Z：戻す / R：やり直す";
        return false;
    }
    const Character& active = s.controlThief ? s.thief : s.police;
    if (d == opposite(active.direction)) {
        s.message = "後ろには進めません！";
        return false;
    }
    std::set<Cell> boxes;
    for (auto box : s.boxes) boxes.insert(cell(box.x,box.y));
    const int td = s.controlThief ? d : opposite(d);
    const int pd = d;
    Intent t = plan(b,cell(s.thief.x,s.thief.y),td,boxes);
    Intent p = plan(b,cell(s.police.x,s.police.y),pd,boxes);

    // An occupied destination can block a moving chain if the character ahead
    // cannot vacate it. Conversely, opposing moving characters can crush a box.
    // Re-evaluate until the two movement flags stabilize.
    for (int iteration=0; iteration<4; ++iteration) {
        bool tm=t.moving, pm=p.moving;
        auto blocks = [&](const Intent& a, const Intent& other) {
            if (!a.moving) return false;
            Cell tip = a.chain.empty() ? a.destination : nextCell(a.chain.back(),a.direction);
            if (tip != other.origin) return false;
            // Characters may overlap: meeting is resolved as capture, even
            // when the other character cannot move because of a wall.
            if (a.chain.empty()) return false;
            // A box arriving at an opposing moving character is destroyed.
            if (!a.chain.empty() && other.moving && other.direction == opposite(a.direction)) return false;
            // Character/box occupying a vacated cell is fine.
            return !other.moving;
        };
        if (blocks(t,p)) t.moving=false;
        if (blocks(p,t)) p.moving=false;
        if (t.moving==tm && p.moving==pm) break;
    }
    // Two opposing pushes into a contiguous chain cannot proceed.
    if (t.moving && p.moving && td==opposite(pd) && !t.chain.empty() && !p.chain.empty()) {
        for (Cell c : t.chain) {
            if (std::find(p.chain.begin(),p.chain.end(),c)!=p.chain.end()) {
                t.moving=false; p.moving=false; break;
            }
        }
    }
    if (!(s.controlThief ? t.moving : p.moving)) {
        s.message = "木箱か壁に阻まれて進めません！";
        return false;
    }
    // Record box movement requests, using old positions as unique identifiers.
    std::map<Cell,int> requests;
    if (t.moving) for (Cell c:t.chain) requests[c]=td;
    if (p.moving) for (Cell c:p.chain) {
        if (requests.count(c) && requests[c]!=pd) {
            // This can only occur for an opposed, immovable chain.
            s.message = "木箱を両側から押しているため動けません！";
            return false;
        }
        requests[c]=pd;
    }
    Cell nt=t.moving?t.destination:t.origin;
    Cell np=p.moving?p.destination:p.origin;
    std::map<Cell,std::vector<Cell>> destinations;
    for (Cell c:boxes) {
        Cell target = requests.count(c) ? nextCell(c,requests[c]) : c;
        destinations[target].push_back(c);
    }
    State next=s;
    next.crashes.clear();
    next.pushes.clear();
    next.boxes.clear();
    std::set<Cell> crashSet;
    for (auto& entry:destinations) {
        Cell target=entry.first;
        const auto& incoming=entry.second;
        bool spike = b.cell[target.second][target.first]=='X';
        bool character = (target==nt || target==np);
        // Stationary boxes do not disappear simply because a character would
        // occupy them; this should have been prevented by the movement check.
        bool incomingBox = false;
        for (Cell c:incoming) incomingBox |= requests.count(c)>0;
        int survivors = static_cast<int>(incoming.size());
        if (spike) survivors=0;
        else if (character && incomingBox) survivors=0;
        else if (survivors>1) survivors=1; // two moving crates collide: destroy one
        if (survivors < static_cast<int>(incoming.size())) crashSet.insert(target);
        if (survivors) next.boxes.push_back({target.first,target.second});
    }
    for (Cell c:crashSet) next.crashes.push_back({c.first,c.second});
    next.thief.x=nt.first; next.thief.y=nt.second;
    next.police.x=np.first; next.police.y=np.second;
    if (t.moving) next.thief.direction=td;
    if (p.moving || (s.controlThief && !p.moving)) next.police.direction=pd;
    if (!s.controlThief && !t.moving) next.thief.direction=td;
    next.message.clear();
    bool spike=b.cell[nt.second][nt.first]=='X' || b.cell[np.second][np.first]=='X';
    bool same=nt==np;
    bool crossed=nt==p.origin && np==t.origin;
    if (spike) {
        next.result=Result::Lose;
        next.message="トゲに触れてしまいました！";
    } else if (same || crossed) {
        next.result=s.controlThief?Result::Lose:Result::Win;
        next.message=s.controlThief?"泥棒が逮捕されました！":"警察が泥棒を逮捕しました！";
    } else if (b.cell[nt.second][nt.first]=='A') {
        next.result=s.controlThief?Result::Win:Result::Lose;
        next.message=s.controlThief?"泥棒がアジトに到着しました！":"泥棒が逃げ切りました！";
    }
    if (next.result==Result::Playing) {
        Cell controlledBefore=s.controlThief?t.origin:p.origin;
        Cell controlledAfter=s.controlThief?nt:np;
        if (controlledBefore!=controlledAfter && b.cell[controlledAfter.second][controlledAfter.first]=='S')
            next.pushes.push_back({controlledAfter.first, controlledAfter.second});
        // Count every crate that ENTERED a switch; resting on it does not count.
        // The destination must still contain that crate after collision resolution.
        for (auto& req:requests) {
            Cell dest=nextCell(req.first,req.second);
            if (b.cell[dest.second][dest.first]!='S') continue;
            if (std::any_of(next.boxes.begin(),next.boxes.end(),[&](Position q){return q.x==dest.first && q.y==dest.second;}))
                next.pushes.push_back({dest.first,dest.second});
        }
        if (next.pushes.size()%2) {
            next.controlThief=!s.controlThief;
            next.message="スイッチで操作が交代しました！";
        } else if (!next.pushes.empty()) {
            next.message="スイッチが複数回押されました！";
        }
    }
    s=std::move(next);
    return true;
}
