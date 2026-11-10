STEP 5 - Five worlds, controls, display settings

IMPORTANT: Make a Git commit before applying this patch.

1) In ~/projects/thief_puzzle, extract the ZIP using:
   unzip -o thief_puzzle_worlds_settings.zip

2) Old stage01.txt ... stage10.txt are not used anymore. The new files are:
   stage1-1.txt, stage1-2.txt, ..., stage5-2.txt
   They preserve the original contents and progression order.
   Experimental stage*_fixed.txt files are ignored.

3) Configure and build:
   cmake -S . -B build
   cmake --build build
   ./build/thief_puzzle
   ./build/thief_puzzle_dev

World names are defined in src/Menu.cpp (worldNames).
World selection: Start -> World selection -> Stage selection -> Playing.
Each world shows three lamps: all stages unlocked / all possible goals completed / all possible goals optimal.
The ending conditions remain global across all worlds.

Settings: remap eight game actions (duplicate/invalid keys are rejected), windowed/fullscreen,
window size, vsync. Audio is a disabled placeholder. No settings or progress saved to disk.

In-game default keys: WASD movement, Z undo, R restart, Q stage select, N next unlocked stage.
N moves across world boundaries when the next global stage is unlocked.
HUD, game instructions, and result popup use the currently assigned keys.

Developer shortcuts F1-F10 remain available on the stage selection screen.
Use F4/F5/F6 for testing the three endings. F7 resets progress.

NOTE: SFML 3 GUI compilation and interactive display were not available in the author's environment.
If a compile error appears, please paste the full error message.
