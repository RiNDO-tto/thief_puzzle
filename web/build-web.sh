#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v em++ >/dev/null 2>&1; then
  echo 'Emscripten (em++) が見つかりません。emsdk を有効化してください。' >&2
  exit 1
fi
mkdir -p web/dist
# Generate stage catalog automatically from the source stage directory.
python3 - <<'PYCODE'
import json, pathlib, re
stages = sorted((p.name for p in pathlib.Path('stages').glob('stage*-*.txt')
                 if re.fullmatch(r'stage[1-6]-[1-9][0-9]*\.txt', p.name)),
                key=lambda n: tuple(map(int, re.findall(r'\d+', n))))
pathlib.Path('web/dist/stages.json').write_text(json.dumps(stages, ensure_ascii=False), encoding='utf-8')
print(f'Bundling {len(stages)} stages')
PYCODE
em++ -std=c++17 -O2 src/Game.cpp web/bridge.cpp \
  -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='["_game_init","_game_load","_game_state","_game_move","_game_undo","_game_reset"]' \
  -s EXPORTED_RUNTIME_METHODS='["UTF8ToString","ccall"]' \
  --preload-file stages@/stages \
  -o web/dist/game.js
cp web/index.html web/play.js web/dist/
mkdir -p web/dist/assets
cp web/assets/* web/dist/assets/
echo 'web/dist/ に出力しました。python3 -m http.server 8000 --directory web/dist で確認できます。'
