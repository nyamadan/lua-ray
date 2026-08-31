---
type: Lua Scene Contract
title: Luaシーンモジュール契約
description: scenes配下のモジュールがレンダーとポストエフェクトへ提供するコールバック契約。
tags: [lua, scenes, callbacks, lifecycle]
status: stable
generated: { by: process:initial-okf-specification, at: 2026-08-30T00:00:00Z }
verified: { by: process:initial-okf-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: ray-worker
    resource: ../../workers/ray_worker.lua
    title: レイワーカー
  - id: post-worker
    resource: ../../workers/posteffect_worker.lua
    title: ポストエフェクトワーカー
  - id: scenes
    resource: ../../scenes/color_pattern.lua
    title: 標準シーン実装
  - id: test-lifecycle
    resource: ../../scenes/test_lifecycle.lua
    title: ライフサイクルテストシーン
  - id: raytracer
    resource: ../../lib/RayTracer.lua
    title: シーン呼び出し側
---

# Module shape

シーンファイルは `return M` するLuaモジュールで、実行時に `scenes.<scene_type>` としてロードされる。`setup`、`start`、`shade`、`get_camera`、`cleanup`、`post_effect`、`stop` は用途に応じて実装する。

# Callbacks

| Callback                      | 呼び出し                                        | 契約                                                                  |
| ----------------------------- | ----------------------------------------------- | --------------------------------------------------------------------- |
| `setup(scene, app_data)`      | シーン作成時にメインStateで一度                 | Embree geometryと共有初期データを作り、シーンは呼び出し側がcommitする |
| `start(scene, app_data)`      | シーン開始時、または各worker State              | カメラ、乱数、State固有値を初期化する                                 |
| `shade(app_data, x, y)`       | レンダーブロック内の各ピクセル                  | `AppData:set_pixel` 等で対象ピクセルを書き込む                        |
| `post_effect(app_data, x, y)` | 任意のポストエフェクトブロック内                | 前段の結果を読み、バックバッファへ処理結果を書く                      |
| `get_camera()`                | UI操作・worker初期化時                          | カメラまたはnilを返し、存在時は状態をJSON化できる構造を持つ           |
| `cleanup(scene)`              | シーン切替前                                    | シーン固有のメインState資源を解放する                                 |
| `stop(scene)`                 | レンダー/ポストエフェクト終了またはworker終了時 | State固有の後処理を行う                                               |
| `is_progressive()`            | シーン開始・pass完了時                          | progressive sceneならtrueを返す                                       |
| `get_max_samples()`           | progressive pass完了時                          | 停止するsample数を正整数で返す                                        |

# Worker invariants

`ray_worker.lua` は `start` の後に共有キューからブロックを取り、各ピクセルで `shade` を呼ぶ。`posteffect_worker.lua` は同じ方式で `post_effect` を呼ぶ。キャンセル関数が真になった場合は処理を中断し、Luaエラーはworker単位で報告後に完了状態へ進む。`shade` と `post_effect` は複数Stateから並行して呼ばれるため、Lua Stateローカル値以外の共有状態を書き換えない。
