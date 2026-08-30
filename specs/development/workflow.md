---
type: Development Workflow
title: 開発・検証ワークフロー
description: lua-rayのビルド、テスト、仕様同期、レビュー前に確認する現行手順。
tags: [development, build, test, cmake, pnpm]
status: stable
generated: { by: process:dependency-upgrade, at: 2026-08-30T12:49:58Z }
verified:
  { by: process:dependency-upgrade-verification, at: 2026-08-30T12:49:58Z }
sources:
  - id: agents
    resource: ../../AGENTS.md
    title: リポジトリガイドライン
  - id: readme
    resource: ../../README.md
    title: 開発者向けREADME
  - id: package
    resource: ../../package.json
    title: pnpm scripts
  - id: cmake
    resource: ../../CMakeLists.txt
    title: CMake依存定義
---

# Commands

```text
pnpm install
cmake --build --preset gcc-debug
ctest --preset gcc-debug --output-on-failure
pnpm build
pnpm test
cmake --build --preset emscripten-debug
pnpm build:emscripten
pnpm start
```

ネイティブの標準検証はGCC Debug buildとCTest。WASM固有の変更はEmscripten buildも行う。GUI起動は `./build/gcc-debug/lua-ray` で確認できるが、ヘッドレス環境では実行不能を失敗と断定せず、build/test結果と理由を報告する。

# Change protocol

変更は [OKF仕様書駆動開発スキル](../../.agents/skills/okf-spec-driven-development/SKILL.md) の手順で、変更仕様、受入条件、テスト、実装、仕様同期の順に進める。機能や振る舞いに影響しない文書変更でも、OKF構造とリンクを検証する。

# Review gates

- C++はC++17、4スペース、説明的なcamelCaseを基本とする。
- SDL、Lua、Embreeの失敗経路とリソース終了順を確認する。
- ネイティブ/WASMの互換性、スレッド安全性、キャンセルを影響範囲に応じて確認する。
- Conventional Commitやpush/PRはユーザーが明示的に依頼した場合だけ行う。この仕様化作業はリモートへ変更を送信しない。
