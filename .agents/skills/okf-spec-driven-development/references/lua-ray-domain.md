# lua-ray ドメインガイド

## 仕様の入口

- 現行仕様の束: `specs/index.md`
- C++実装: `src/`
- Luaシーン: `scenes/`
- Lua共有ライブラリ: `lib/`
- ワーカー: `workers/`
- C++テスト: `test/`
- ビルド・実行の一次資料: `README.md`、`CMakeLists.txt`、`CMakePresets.json`、`package.json`

## 重要な観測点

- シーンは `setup`（メインスレッドでジオメトリ構築）、`start`（状態初期化）、`shade`（ピクセル）、任意の `post_effect`、`get_camera`、`cleanup`/`stop` を持つ。
- `AppData` はfront/backのRGBAバッファ、文字列ストレージ、glTF/テクスチャキャッシュを提供する。`pop_next_index` が共有キューのインデックス取得に使われる。
- `ThreadWorker` はLua Stateをスレッドごとに作り、共有の `AppData`/`EmbreeScene` と `_bounds`、`_scene_type`、キャンセル関数を注入する。`terminate` はキャンセル要求後にjoinする。
- `RayTracer` はシーン切替・解像度変更・キャンセル時にワーカー/コルーチンを停止し、ブロックキューを再構築する。レンダー後は任意のポストエフェクトを経てバッファをswapする。
- C++/Lua境界の公開型・関数は `src/lua_binding.cpp` の `bind_common_types`、`bind_lua`、`bind_worker_lua` が一次資料である。

## 検証コマンド

```text
cmake --build --preset gcc-debug
ctest --preset gcc-debug --output-on-failure
pnpm exec prettier --check "specs/**/*.md" ".agents/skills/**/*.md" ".agents/skills/**/*.yaml"
python3 /home/vscode/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/okf-spec-driven-development
```

Emscripten固有の変更は `cmake --build --preset emscripten-debug` も実行する。アプリ起動はGUI/SDL環境が必要なため、ヘッドレス環境ではビルドとテストの結果を代替証拠とする。
