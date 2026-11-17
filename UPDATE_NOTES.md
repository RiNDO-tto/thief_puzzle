# Update: crate animation / developer build / CLICK delay

## Files changed
- `src/Game.hpp`, `src/Game.cpp`: record surviving box movements in transient `State::boxMotions` (no change to move rules).
- `src/Renderer.hpp`, `src/Renderer.cpp`: render moving boxes at interpolated positions over 0.16 seconds.
- `src/main.cpp`: restart animation clock on successful moves; delay CLICK by 0.3 seconds **only when a CRASH occurred at the same cell**. Other CLICK events remain immediate.
- `CMakeLists.txt`: optional `thief_puzzle_dev` target with `THIEF_PUZZLE_DEV` enabled. Existing normal target unchanged.

## Build and run (project root)
```
cmake -S . -B build
cmake --build build
./build/thief_puzzle
./build/thief_puzzle_dev
```

Developer shortcuts F1-F10 are in the stage-selection screen. Both builds use the same box-aware `Game.cpp` and `Solver.cpp`. `main4.cpp` is already box-aware in the supplied archive. To build modifier:
```
g++ -std=c++17 -Isrc src/Game.cpp main4.cpp -o modifier
```

## Notes
- Box animation is visual only; BFS and game mechanics are unchanged.
- Animation can be interrupted by a rapid second input, and Undo/restart immediately shows the restored board.
- If assets are in `assets/images/`, continue running from project root.
- GUI compilation was not verified in the patch environment because SFML 3 headers/libs were not installed. `Game.cpp`, `main3-2.cpp`, and `main4.cpp` compiled successfully with C++17.
