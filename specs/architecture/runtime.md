---
type: Runtime Architecture
title: ランタイム構成
description: C++ホスト、Lua State、SDLメインループ、ワーカーの所有権と終了順序。
tags: [runtime, lifecycle, threading, sdl]
status: stable
generated: { by: process:initial-okf-specification, at: 2026-08-30T00:00:00Z }
verified: { by: process:initial-okf-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: main
    resource: ../../src/main.cpp
    title: C++エントリーポイント
  - id: app
    resource: ../../src/app.cpp
    title: SDL/ImGuiメインループ
  - id: worker
    resource: ../../src/thread_worker.cpp
    title: Luaワーカー実装
  - id: context
    resource: ../../src/lua_binding.h
    title: アプリケーションコンテキスト
---

# Startup

C++エントリーポイントはLua Stateとバインディングを初期化し、引数があればそのファイル、なければ `main.lua` を実行する。Lua側の `RayTracer:init` はSDL video、window、renderer、streaming texture、`AppData`、Embree deviceの順で準備し、`app.configure` へハンドルを渡す。

# Main loop

SDLイベントをImGuiへ転送し、`app.on_frame` を毎フレーム呼び、テクスチャをアスペクト比維持で描画してImGuiをpresentする。終了イベントでは `app.on_quit` を一度呼び、ネイティブではループを抜け、Emscriptenではブラウザのmain loopをキャンセルする。

# Lua states and ownership

メインStateはSDL、ImGui、ThreadWorkerを含む全バインディングを持つ。各 `ThreadWorker` は独立したLua Stateを作り、worker用の限定バインディング、同じ `AppData`/`EmbreeScene` ポインタ、シーン種別、キャンセル関数を注入する。LuaテーブルはState間で共有せず、必要な値は文字列（現行コードではJSON）またはC++のreadonlyキャッシュを使う。

# Shutdown and reset

シーン切替、解像度変更、モード切替、終了では先にワーカーを `terminate` してjoinし、コルーチンを破棄する。シーン切替では現シーンの `cleanup`、Embree sceneのrelease、新sceneのsetup/start/commitを順に行い、終了時はワーカーを安全に停止する。
