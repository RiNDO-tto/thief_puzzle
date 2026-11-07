#pragma once
#include <array>
#include <string>
#include <vector>

enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };
extern const int dx[4];
extern const int dy[4];
enum class Result { Playing, Win, Lose };
struct Character { int x, y, direction; };
struct State {
    Character thief;
    Character police;
    bool controlThief = true;
    Result result = Result::Playing;
    std::string message;
};
struct Board {
    int width = 0;
    int height = 0;
    std::vector<std::vector<char>> cell;
    std::vector<std::vector<std::array<bool, 4>>> wall;
};
int opposite(int d);
bool isInside(const Board& b, int x, int y);
int arrowDirection(char c);
bool canMove(const Board& b, const Character& c, int d);
bool isStuck(const Board& b, const State& s);
bool loadStage(const std::string& filename, Board& b, State& s);
bool takeTurn(const Board& b, State& s, int d);
