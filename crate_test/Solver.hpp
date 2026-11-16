#pragma once
#include "Game.hpp"
struct Reachability { bool thief = false; bool police = false; };
struct ShortestMoves { int thief = -1; int police = -1; }; // -1 means unreachable
ShortestMoves solveShortestMoves(const Board& board, const State& initial);
Reachability solveReachability(const Board& board, const State& initial);
