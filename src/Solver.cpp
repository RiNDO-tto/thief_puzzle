#include "Solver.hpp"
#include <queue>
#include <unordered_set>
#include <string>

namespace {
std::string key(const State& s) {
    return std::to_string(s.thief.x) + "," + std::to_string(s.thief.y) + "," +
        std::to_string(s.thief.direction) + "," + std::to_string(s.police.x) + "," +
        std::to_string(s.police.y) + "," + std::to_string(s.police.direction) + "," +
        (s.controlThief ? "T" : "P");
}
}
Reachability solveReachability(const Board& board, const State& initial) {
    Reachability result;
    std::queue<State> pending;
    std::unordered_set<std::string> visited;
    pending.push(initial);
    visited.insert(key(initial));
    while (!pending.empty() && !(result.thief && result.police)) {
        State current = pending.front();
        pending.pop();
        for (int d = 0; d < 4; ++d) {
            State next = current;
            if (!takeTurn(board, next, d)) continue;
            if (next.result == Result::Win) {
                if (current.controlThief) result.thief = true;
                else result.police = true;
            } else if (next.result == Result::Playing && visited.insert(key(next)).second) {
                pending.push(std::move(next));
            }
        }
    }
    return result;
}
