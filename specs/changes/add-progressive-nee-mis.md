---
type: Change Specification
title: Progressive NEE/MISパストレーシング
description: DamagedHelmetへ決定的なprogressive path tracingとNEE/MISを追加する。
tags: [rendering, path-tracing, mis, progressive]
status: stable
generated: { by: process:codex-implementation, at: 2026-08-30T00:00:00Z }
verified: { by: process:codex-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: path-tracer
    resource: ../../lib/PathTracer.lua
    title: パストレーサー
  - id: helmet
    resource: ../../scenes/damaged_helmet.lua
    title: DamagedHelmet PBRシーン
---

# Intent

利用者が既存の高速PBR表示と比較しながら、DamagedHelmetを間接光、矩形面光源の次イベント推定、MISを含むMonte Carlo path tracingでprogressive描画できるようにする。

# Non-goals

双方向パストレーシング、媒体、屈折、環境マップimportance sampling、GPU実装は扱わない。

# Acceptance Criteria

- [x] AppDataのlinear RGB累積平均とsample countがreset可能で、異なるpixelへの並行加算が成立する。
- [x] 決定的RNGが同じpixel/passで同じ列、異なるpassで異なる列を返す。
- [x] Lambert/GGX BSDFのevaluate/sample/pdfが有限かつ非負で、MIS power heuristicが境界値を処理する。
- [x] NEE+MIS積分器が遮蔽、発光面hit、Russian rouletteを処理し、固定seedで再現可能な値を返す。
- [x] `Damaged Helmet PT` が1 sample/passを累積し、128 sppで停止する。
- [x] カメラ、解像度、シーン変更とキャンセルでprogressive累積を破棄する。
- [x] GCC build、CTest、Emscripten buildが成功する。

# Affected Concepts

- [Luaバインディング](/architecture/bindings.md)
- [レンダリングパイプライン](/architecture/rendering-pipeline.md)
- [ライブラリとアセット](/architecture/library-and-assets.md)
- [Luaシーン契約](/architecture/lua-scene-contract.md)

# Implementation Notes

既存のDamagedHelmet PBRシーンは維持する。float累積値はAppDataに置き、各passのpixel所有権を既存block queueで排他的にする。
