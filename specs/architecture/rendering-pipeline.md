---
type: Rendering Pipeline
title: レンダリングパイプライン
description: ブロックキュー、シングル/マルチスレッド、ダブルバッファ、ポストエフェクトの現行処理。
tags: [rendering, threading, blocks, buffers, post-effect]
status: stable
generated: { by: process:codex-implementation, at: 2026-08-30T00:00:00Z }
verified:
  - { by: process:codex-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: raytracer
    resource: ../../lib/RayTracer.lua
    title: パイプライン制御
  - id: render-stage
    resource: ../../lib/RenderStage.lua
    title: 共通レンダーステージ制御
  - id: blocks
    resource: ../../lib/BlockUtils.lua
    title: ブロック・キュー実装
  - id: worker-utils
    resource: ../../workers/worker_utils.lua
    title: ブロック処理
  - id: app-data
    resource: ../../src/app_data.h
    title: ダブルバッファ
  - id: worker-tests
    resource: ../../test/worker_utils_test.cpp
    title: ワーカーユーティリティテスト
  - id: render-stage-tests
    resource: ../../test/render_stage_test.cpp
    title: レンダーステージテスト
---

# Render stages

`RayTracer:render` は両バッファをクリアし、`render_queue` を作り、シングルスレッドではコルーチン、マルチスレッドでは `ThreadWorker` 群を開始する。各ブロックは共有キューの `pop_next_index` で一度だけ取得され、コールバックがブロック内のピクセルを処理する。

描画とポストエフェクトに共通するキュー準備、worker群の生成・完了判定、コルーチンのブロック処理は `RenderStage` が担い、`RayTracer` は各ステージ固有のcallbackと完了処理を渡す。

描画中はback bufferをテクスチャへ表示し、全workerまたはコルーチンが完了したら、ポストエフェクトがあればそのステージへ移る。なければ `swap`、back bufferクリア、front bufferのテクスチャ更新を行う。

`is_progressive()` を返すシーンでは、各passが1 sample/pixelをlinear累積バッファへ加算する。pass完了後は平均画像を表示し、frontをbackへコピーして次passを開始する。`get_max_samples()` 到達時に停止し、カメラ・解像度・シーン変更とキャンセルでは累積をresetする。

# Post-effect stage

ポストエフェクト開始時は完成した画像を読み取り側へswapし、空のback bufferへ結果を書く。`posteffect_queue` をシングル/マルチスレッドで処理し、完了後にswapしてテクスチャを更新する。

# Cancellation and progress

`cancel` は全workerへキャンセル要求を送り、joinし、コルーチンを破棄する。カメラ、解像度、スレッド数、ブロックサイズ、シーンを変更する操作は実行中処理を停止してから再描画する。workerは完了時に `is_done=true`、`get_progress=1.0` へ進む。

# Safety properties

各ブロックは排他的に担当され、`AppData:set_pixel` と `accumulate_sample` は担当範囲を前提に異なるpixelへ書く。`AppData` の文字列ストレージとresource cacheはmutexで保護される。Embree sceneはsetup/commit後に交差判定をreadonly共有する。
