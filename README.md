# Thief Puzzle (SFML 3)

Refactored from `main2-2.cpp` and `main_gui_updated.cpp`.

- `Game.hpp` / `Game.cpp`: original rules and ASCII stage loader
- `Renderer.hpp` / `Renderer.cpp`: SFML rendering and result overlay
- `main.cpp`: window, input, undo, restart

Place your existing `stages/` directory in this folder.

Build on Ubuntu/WSL:

```bash
cmake -S . -B build
cmake --build build
./build/thief_puzzle stages/stage01.txt
```

WASD: move, Z: undo, R: restart, Q/Esc: quit.
