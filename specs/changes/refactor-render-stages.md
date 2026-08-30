---
type: Change Specification
title: RayTracerレンダーステージの共通化
description: 描画とポストエフェクトに重複するワーカー・コルーチン制御を内部モジュールへ集約する。
tags: [refactoring, rendering, threading, coroutine]
status: stable
generated: { by: process:render-stage-refactoring, at: 2026-08-30T22:54:00Z }
verified:
  - {
      by: process:render-stage-refactoring-verification,
      at: 2026-08-30T22:54:00Z,
    }
sources:
  - id: pipeline
    resource: /architecture/rendering-pipeline.md
    title: レンダリングパイプライン
  - id: raytracer
    resource: ../../lib/RayTracer.lua
    title: RayTracer実装
  - id: render-stage
    resource: ../../lib/RenderStage.lua
    title: 共通レンダーステージ実装
  - id: render-stage-tests
    resource: ../../test/render_stage_test.cpp
    title: レンダーステージテスト
---

# Intent

`RayTracer` の描画とポストエフェクトに重複するワーカー生成、完了判定、コルーチン処理を内部モジュールへ集約し、挙動を変えずに保守しやすくする。

# Non-goals

- 公開Lua API、C++バインディング、worker scriptの契約は変更しない。
- single/multithread間で異なるバッファ更新やscene `stop` のタイミングは統一しない。
- Emscripten固有コード、依存関係、UIは変更しない。

# Acceptance Criteria

- [x] `RenderStageTest.StartWorkersCreatesConfiguredWorkerGroup` が、共有キューの準備と指定数のworker起動を確認する。
- [x] `RenderStageTest.AllWorkersDoneRequiresEveryWorkerToFinish` が、worker群の完了判定を確認する。
- [x] `RenderStageTest.CoroutineProcessesBlocksAndRunsCompletionCallback` が、キュー処理、ブロック単位yield、完了callbackを確認する。
- [x] 既存の `RayTracerTest` を含む `ctest --preset gcc-debug --output-on-failure` が成功する。
- [x] GCC Debug build、OKF構造・リンク、Markdown/YAML、スキル検証が成功する。

# Affected Concepts

- [レンダリングパイプライン](/architecture/rendering-pipeline.md)

# Implementation Notes

`lib/RenderStage.lua` に `start_workers`、`all_workers_done`、`create_coroutine` を定義し、`lib/RayTracer.lua` のrender/post-effect固有メソッドから利用する。テストは `test/render_stage_test.cpp` に追加する。
