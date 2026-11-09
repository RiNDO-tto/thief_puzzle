#pragma once
#include "Game.hpp"
struct Reachability { bool thief = false; bool police = false; };
Reachability solveReachability(const Board& board, const State& initial);
