#pragma once
#include "Game.hpp"
#include "Menu.hpp"
#include <SFML/Graphics.hpp>

void drawBoard(sf::RenderWindow& window, const Board& board, const State& state);
void drawResultOverlay(sf::RenderWindow& window, const State& state);
void updateTitle(sf::RenderWindow& window, const State& state);

void drawHud(sf::RenderWindow& window, const Board& board, const State& state,
             std::size_t moves, const std::string& stageName, const StageProgress& progress);
