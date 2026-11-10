#include "Solver.hpp"
#include <queue>
#include <unordered_set>
#include <string>
#include <utility>

namespace {
std::string key(const State& s) {
    return std::to_string(s.thief.x) + "," + std::to_string(s.thief.y) + "," +
        std::to_string(s.thief.direction) + "," + std::to_string(s.police.x) + "," +
        std::to_string(s.police.y) + "," + std::to_string(s.police.direction) + "," +
        (s.controlThief ? "T" : "P");
}
}
ShortestMoves solveShortestMoves(const Board& board, const State& initial) {
    ShortestMoves result;
    std::queue<std::pair<State, int>> pending;
    std::unordered_set<std::string> visited;
    pending.push({initial, 0});
    visited.insert(key(initial));
    while (!pending.empty() && (result.thief < 0 || result.police < 0)) {
        auto [current, moves] = pending.front();
        pending.pop();
        for (int d = 0; d < 4; ++d) {
            State next = current;
            if (!takeTurn(board, next, d)) continue;
            if (next.result == Result::Win) {
                int& best = current.controlThief ? result.thief : result.police;
                if (best < 0) best = moves + 1;
            } else if (next.result == Result::Playing && visited.insert(key(next)).second) {
                pending.push({std::move(next), moves + 1});
            }
        }
    }
    return result;
}
Reachability solveReachability(const Board& board, const State& initial) {
    const auto shortest = solveShortestMoves(board, initial);
    return {shortest.thief >= 0, shortest.police >= 0};
}
