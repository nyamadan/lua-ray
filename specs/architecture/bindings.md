---
type: Lua Binding API
title: C++からLuaへのバインディング
description: AppData、Embree、glTF、ThreadWorker、SDL/入力/テクスチャ操作のLua公開面。
tags: [lua, api, cpp, appdata]
status: stable
generated: { by: process:initial-okf-specification, at: 2026-08-30T00:00:00Z }
verified: { by: process:initial-okf-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: binding-header
    resource: ../../src/lua_binding.h
    title: バインディング宣言
  - id: binding-impl
    resource: ../../src/lua_binding.cpp
    title: バインディング実装
  - id: app-data
    resource: ../../src/app_data.h
    title: AppData実装
  - id: binding-tests
    resource: ../../test/lua_binding_test.cpp
    title: バインディングテスト
---

# Common types

- `AppData.new(width, height)` はfront/back RGBAバッファを作る。`set_pixel` は範囲外を無視し、`get_pixel` はfront bufferを読む。`swap`、`copy_front_to_back`、`copy_back_to_front`、`clear`、`clear_back_buffer`、`width`、`height` を提供する。
- `AppData` は排他制御付きの `set_string`、`get_string`、`has_string`、`pop_next_index` と、glTF/texture cacheの `load_*`、`get_*` を提供する。
- `EmbreeDevice.new():create_scene()` は `EmbreeScene` を作る。sceneは `add_sphere`、`add_triangle`、`add_mesh`、`commit`、`intersect`、`release` を提供する。`intersect` はhit、距離、法線、geometry/primitive ID、barycentric値を返す。
- `GltfData.new()` は `load`、`is_loaded`、mesh/vertex/index/UV/texture取得、`release` を提供する。失敗または存在しないcache項目はnil/空値として扱う。

# Main app namespace

メインStateの `app` はSDL video/window/renderer/texture作成、`configure`、テクスチャlock/unlock・更新・破棄、ticks、キーボード状態、マウス状態を提供する。SDLまたは無効ハンドルで失敗した生成関数はnil/falseを返し、エラーを標準エラーへ出す。ImGuiが入力を捕捉している場合は入力状態を空またはfalseとして返す。

# Worker namespace

worker Stateの `app` は共通型と `get_ticks` のみを持つ。`ThreadWorker.create(data, scene, x, y, w, h, id)`、`start(script_path, scene_type)`、`join`、`terminate`、`is_done`、`is_cancel_requested`、`get_progress` を使ってworkerのライフサイクルを制御する。
