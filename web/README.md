# Thief and Police — Web edition

Web版は既存の `src/Game.cpp` / `src/Game.hpp` を使用します。PC版のSFMLコードには変更不要です。

## インストール

プロジェクトルートでこのアーカイブを展開してください。`web/` と `stages/` が追加・更新されます。

## ビルド

```bash
source ~/emsdk/emsdk_env.sh
./web/build-web.sh
python3 -m http.server 8000 --directory web/dist
```

ブラウザで http://localhost:8000 を開いてください。

## 操作

WASD / 矢印キー: 移動、Z: 一手戻す、R: リセット。
ワールド・ステージを選んで「プレイ」を押すと切り替わります。「次のステージ」は番号順で次のステージを開きます。

## ステージ追加

`stages/stage6-4.txt` のような形式で追加して再ビルドするだけで一覧に反映されます。`stages.json` はビルド時に生成されます。

## 注意

Emscriptenでの実際のWebAssemblyビルドとブラウザ表示は、この生成環境では未検証です。C++17で14ステージすべての読み込み、JavaScript構文、シェル構文は確認済みです。
