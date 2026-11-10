STEP 4 - Developer executable

Apply this archive to the existing project (after STEP 3 + ending condition fix).
The archive replaces ONLY CMakeLists.txt and src/main.cpp.

Build:
  cmake -S . -B build
  cmake --build build

Normal game:
  ./build/thief_puzzle

Developer game:
  ./build/thief_puzzle_dev

In the developer game's STAGE SELECTION screen:
  F1   selected stage: thief objective, optimal
  F2   selected stage: police objective, optimal
  F3   selected stage: both possible objectives, optimal
  F4   all stages: one possible objective each, non-optimal
  F5   all stages: all possible objectives, non-optimal
  F6   all stages: all possible objectives, optimal
  F7   reset session progress and ending notification history
  F8   preview normal ending
  F9   preview true ending
  F10  preview perfect ending

After F4/F5/F6, open any stage and press Q to trigger the normal
ending progression check. F8/F9/F10 are preview-only and do not
change the highest ending shown by automatic progression.

Unavailable objectives are ignored. Progress is memory-only.
Developer shortcuts are not compiled into the normal game.
