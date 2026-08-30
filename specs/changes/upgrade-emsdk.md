---
type: Change Specification
title: emsdk 6.0.5への更新
description: Emscriptenツールチェーンを再現可能な最新安定版へ更新する。
tags: [dependencies, emsdk, emscripten, wasm]
status: stable
generated: { by: process:emsdk-upgrade, at: 2026-08-30T13:05:57Z }
verified: { by: process:emsdk-upgrade-verification, at: 2026-08-30T13:05:57Z }
sources:
  - id: ci
    resource: ../../.github/workflows/ci.yml
    title: CIワークフロー
  - id: devcontainer
    resource: ../../.devcontainer/install.sh
    title: devcontainerセットアップ
  - id: submodule
    resource: ../../.gitmodules
    title: emsdk submodule定義
  - id: workflow
    resource: /development/workflow.md
    title: 開発・検証ワークフロー
---

# Intent

emsdkとEmscriptenを4.0.22から6.0.5へ更新し、ローカル開発とCIで同じ再現可能なWebAssemblyツールチェーンを使用する。既存のネイティブおよびWebAssembly向け成果物を維持する。

# Non-goals

- 公開Lua API、シーン契約、レンダリング動作は変更しない。
- Emscripten以外の依存ライブラリとGitHub Actionsは更新しない。
- 浮動する`latest`エイリアスは使用しない。

# Acceptance Criteria

- [x] `.emsdk` submoduleが公式タグ6.0.5のコミットを指す。
- [x] CIとdevcontainerがEmscripten 6.0.5を厳密に指定する。
- [x] `emcc --version`が6.0.5を報告する。
- [x] Emscripten Debug buildが成功する。
- [x] Emscripten Release buildが成功し、HTML、JavaScript、WASM、data成果物が生成される。
- [x] GCC Debug buildとCTestが成功する。
- [x] OKF構造、リンク、Markdownの検証が成功する。

# Affected Concepts

- [システム概要](/architecture/system.md)
- [開発・検証ワークフロー](/development/workflow.md)

# Implementation Notes

`.emsdk`を公式タグ6.0.5のコミットに固定し、CIとdevcontainerのインストール・有効化バージョンを揃える。Emscripten 6.0.5による互換修正が必要な場合は、既存動作を変えない最小限の変更に限定する。

# Verification Result

`emcc --version`で6.0.5を確認した。Emscripten Debug/Release build、GCC Debug build、CTest、OKF構造・リンク・Markdown検証はすべて成功し、Release buildでは`lua-ray.html`、`lua-ray.js`、`lua-ray.wasm`、`lua-ray.data`が生成された。製品コードの互換修正は不要だった。

CMake 4.2.3とEmscripten 6.0.5の組み合わせでは共有ライブラリ非対応の警告が出るが、WASM buildはSDL、Embree、Lua、ImGuiを静的リンクして完了するため、本変更の成果物には影響しない。
