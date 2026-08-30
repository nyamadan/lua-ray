---
type: Rendering Libraries and Assets
title: Luaライブラリとアセット
description: カメラ、ベクトル、レイ、マテリアル、テクスチャ、glTF、プリセットの再利用可能な境界。
tags: [lua, library, gltf, texture, camera]
status: stable
generated: { by: process:initial-okf-specification, at: 2026-08-30T00:00:00Z }
verified: { by: process:initial-okf-verification, at: 2026-08-30T00:00:00Z }
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
---

# Lua libraries

`Vec3`、`Ray`、`Camera`、`Material`、`Texture`、`PathTracer`、`RayTracer` は `lib/` からrequireする。カメラは透視/正投影レイ生成、移動、回転、状態取得を担い、マテリアルはLambertian、Metal、Dielectric、DiffuseLightを提供する。`BlockUtils`、`ThreadPresets`、`ResolutionPresets` は並列実行とUI設定を支える。

# glTF and textures

`GltfData` はglTF/GLBをRAII管理し、mesh数、primitiveの頂点・index・TEXCOORD_0、デコード済み画像を取得する。`AppData` のcacheはロード済み名をキーにし、同名の再ロードを省略する。テクスチャ画像は幅、高さ、チャンネル数、byte pixel配列としてLuaへ返す。

# Scene assets

`scenes/` のサンプルは、単純なtriangle/sphere、マテリアル、WASDカメラ、ポストエフェクト、material transfer、glTF box、Cornell box、Ray Tracing in One Weekendを含む。実行時に参照する `main.lua`、`workers/`、`lib/`、`scenes/`、`assets/` はEmscripten buildでpreloadされる。
