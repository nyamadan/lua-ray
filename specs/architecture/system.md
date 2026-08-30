---
type: System Architecture
title: lua-ray システム概要
description: C++ホストとLuaスクリプトを組み合わせたEmbreeレイトレーシングアプリケーションの現行構成。
tags: [architecture, cpp, lua, embree, sdl, wasm]
status: stable
generated: { by: process:dependency-upgrade, at: 2026-08-30T12:49:58Z }
verified:
  { by: process:dependency-upgrade-verification, at: 2026-08-30T12:49:58Z }
sources:
  - id: readme
    resource: ../../README.md
    title: プロジェクトREADME
  - id: introduction
    resource: ../../docs/introduction.md
    title: プロジェクト紹介
  - id: cmake
    resource: ../../CMakeLists.txt
    title: ビルド構成
---

# Purpose

`lua-ray` は、C++17をホスト、Lua 5.4.9をシーンとレンダリングロジック、Embree 4.4.1をレイ交差、SDL3/ImGuiを表示と入力に使う実験的なレイトレーシングアプリケーションである。

# Responsibilities

- C++はSDL初期化、メインループ、Lua State、スレッド、AppData、Embree、glTFのリソース境界を管理する。
- Luaはシーン定義、カメラ、マテリアル、ピクセル処理、ポストエフェクト、UI操作を記述する。
- Embreeはジオメトリをcommitし、ワーカーからの交差判定を処理する。
- `AppData` はレンダー結果とスレッド間で必要な文字列・readonlyリソースを共有する。

# Targets

GCCによるネイティブ実行と、Emscriptenによるブラウザ向けWebAssemblyを同じソースから構築する。WASMではpthread、Web Worker、SDL3バックエンド、Lua/シーン/アセットのpreloadを使用する。

# Related concepts

詳細な実行順は [ランタイム構成](/architecture/runtime.md)、シーンの外部契約は [Luaシーン契約](/architecture/lua-scene-contract.md)、検証手順は [開発ワークフロー](/development/workflow.md) を参照する。
