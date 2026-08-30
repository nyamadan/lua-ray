---
type: Change Specification
title: 依存ライブラリの更新
description: 直接利用する依存ライブラリを検証可能な最新安定版へ更新する。
tags: [dependencies, cmake, native, wasm]
status: stable
generated: { by: process:dependency-upgrade, at: 2026-08-30T12:49:58Z }
verified:
  { by: process:dependency-upgrade-verification, at: 2026-08-30T12:49:58Z }
sources:
  - id: cmake
    resource: ../../CMakeLists.txt
    title: CMake依存定義
  - id: package
    resource: ../../package.json
    title: Node.js依存定義
  - id: workflow
    resource: /development/workflow.md
    title: 開発・検証ワークフロー
---

# Intent

直接利用する依存ライブラリを最新の安定版へ更新し、ネイティブとWebAssemblyの既存機能を維持する。更新が大規模な製品コード変更を必要とする場合は、その依存だけ現行版を維持する。

# Non-goals

- emsdk、pnpm、Codex、GitHub Actionsなどの開発ツールは更新しない。
- 公開API、Luaシーン契約、データ形式、レンダリング動作は変更しない。
- 安定リリースのない依存を開発ブランチの任意コミットへ動かさない。

# Acceptance Criteria

- [x] SDL、Lua、ImGui、Embree、GoogleTestについて、最新安定版または検証可能なフォールバック版が `CMakeLists.txt` に固定される。
- [x] `cmake --build --preset gcc-debug` が成功する。
- [x] `ctest --preset gcc-debug --output-on-failure` で全テストが成功する。
- [x] `cmake --build --preset emscripten-debug` が成功する。
- [x] READMEとOKF仕様に、採用したLuaおよびEmbreeのバージョンが反映される。
- [x] 見送った依存と理由が本変更仕様に記録される。

# Affected Concepts

- [システム概要](/architecture/system.md)
- [開発・検証ワークフロー](/development/workflow.md)

# Implementation Notes

依存は一つずつ更新してGCCビルドとCTestで回帰を検出する。全候補の統合後にEmscriptenビルドを実行する。互換対応は依存APIの軽微なコンパイル修正に限定し、製品挙動の変更が必要な更新は見送る。

候補はSDL 3.4.14、Lua 5.5.1（失敗時は5.4.9）、ImGui 1.92.9b、Embree 4.4.1、GoogleTest 1.18.0。cgltf 1.15とPrettier 3.9.6は既に最新安定版のため維持する。sol2は現行コミットが最新安定タグより先の開発系列であり、stbは安定タグがないため維持する。

# Verification Result

SDL 3.4.14、Lua 5.4.9、ImGui 1.92.9b、Embree 4.4.1、GoogleTest 1.18.0を採用した。GCC Debug build、CTest、Emscripten Debug build、Win32 Release buildはすべて成功した。

Lua 5.5.1は、現行sol2がLua 5.5を未サポートであり、`lua_newstate`など複数のC API変更への対応を要するため見送った。sol2とstbは上記理由で維持し、cgltfとPrettierは既に最新安定版のため維持した。
