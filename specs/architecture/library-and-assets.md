---
type: Rendering Libraries and Assets
title: Luaライブラリとアセット
description: カメラ、ベクトル、レイ、マテリアル、テクスチャ、glTF、プリセットの再利用可能な境界。
tags: [lua, library, gltf, texture, camera]
status: stable
generated: { by: process:codex-implementation, at: 2026-08-30T00:00:00Z }
verified: { by: process:codex-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: libs
    resource: ../../lib/Camera.lua
    title: カメラライブラリ
  - id: material
    resource: ../../lib/Material.lua
    title: マテリアルライブラリ
  - id: texture
    resource: ../../lib/Texture.lua
    title: テクスチャライブラリ
  - id: gltf
    resource: ../../src/gltf_loader.h
    title: glTFローダー
  - id: scenes
    resource: ../../scenes/gltf_box_textured.lua
    title: テクスチャ付きglTFシーン
  - id: damaged-helmet
    resource: ../../scenes/damaged_helmet.lua
    title: DamagedHelmet PBRシーン
  - id: pbr
    resource: ../../lib/PBR.lua
    title: PBRライブラリ
  - id: pbr-tests
    resource: ../../test/pbr_test.cpp
    title: PBRテスト
  - id: bsdf
    resource: ../../lib/BSDF.lua
    title: Monte Carlo BSDF
  - id: pathtraced-helmet
    resource: ../../scenes/damaged_helmet_pathtraced.lua
    title: DamagedHelmet PTシーン
  - id: monte-carlo-tests
    resource: ../../test/monte_carlo_test.cpp
    title: Monte Carloテスト
---

# Lua libraries

`Vec3`、`Ray`、`Camera`、`Material`、`Texture`、`PathTracer`、`RayTracer` は `lib/` からrequireする。カメラは透視/正投影レイ生成、移動、回転、状態取得を担い、マテリアルはLambertian、Metal、Dielectric、DiffuseLightを提供する。`BlockUtils`、`ThreadPresets`、`ResolutionPresets` は並列実行とUI設定を支える。

# glTF and textures

`GltfData` はglTF/GLBをRAII管理し、mesh数、primitiveの頂点・法線・index・TEXCOORD_0、デコード済み画像を取得する。`AppData` のcacheはロード済み名をキーにし、同名の再ロードを省略する。テクスチャ画像は従来のbyte pixel配列に加え、worker間で共有するreadonlyハンドルとして取得・サンプリングできる。

`PBR` はlinear/sRGB変換、GGX/Smith/SchlickによるCook-Torrance BRDF、ACESトーンマッピングを提供する。DamagedHelmetシーンはglTFのbase color、metallic/roughness、normal、occlusion、emissiveテクスチャをこれらへ入力する。

`RNG` はpixel座標とpass番号から決定的な乱数列を作る。`BSDF` はLambert/GGXのevaluate、sample、PDFとpower heuristicを提供し、`PathTracer.trace_mis` は面光源の次イベント推定、BSDF sampling、MIS、Russian rouletteを組み合わせる。DamagedHelmet PTシーンは矩形面光源と拡散床を加え、128 sppまでprogressive描画する。

# Scene assets

`scenes/` のサンプルは、単純なtriangle/sphere、マテリアル、WASDカメラ、ポストエフェクト、material transfer、glTF box、DamagedHelmet PBR/PT、Cornell box、Ray Tracing in One Weekendを含む。実行時に参照する `main.lua`、`workers/`、`lib/`、`scenes/`、`assets/` はEmscripten buildでpreloadされる。
