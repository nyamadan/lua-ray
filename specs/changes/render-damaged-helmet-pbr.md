---
type: Change Specification
title: DamagedHelmetのPBRレンダリング
description: DamagedHelmet.glbの全PBRテクスチャを使うLuaシーンを追加する。
tags: [rendering, lua, gltf, pbr]
status: stable
generated: { by: process:codex-implementation, at: 2026-08-30T00:00:00Z }
verified: { by: process:codex-verification, at: 2026-08-30T00:00:00Z }
sources:
  - id: asset
    resource: ../../assets/DamagedHelmet.glb
    title: DamagedHelmet GLB
  - id: request
    resource: ../../scenes/damaged_helmet.lua
    title: DamagedHelmet Luaシーン
---

# Intent

利用者がDamagedHelmetを選択し、glTF metallic-roughnessのbase color、metallic/roughness、normal、occlusion、emissiveを反映した画像をレンダリングできるようにする。

# Non-goals

環境マップIBL、多重反射パストレーシング、汎用glTF scene graph/material APIは追加しない。

# Acceptance Criteria

- [x] `GltfLoaderTest.CanExtractDamagedHelmetNormals` が14,556頂点分の法線を取得し、範囲外は空配列になることを確認する。
- [x] `TextureTest.CachedTextureHandleSamplesWithoutPixelCopy` がreadonlyキャッシュハンドルからRGBをサンプリングできることを確認する。
- [x] `PbrTest` がsRGB変換、metallic-roughness BRDF、ACES出力の有限性と範囲を確認する。
- [x] `DamagedHelmetSceneTest` がsetup/start/shade/get_camera/cleanupをエラーなく実行し、中央画素へ背景以外の色を書き込むことを確認する。
- [x] 初期カメラが右上からの三分の四ビュー `position = {1.15, 0.55, 3.0}` を返すことを `DamagedHelmetSceneTest` で確認する。
- [x] GCC build、CTest、Emscripten buildが成功する。

# Affected Concepts

- [Luaバインディング](/architecture/bindings.md)
- [ライブラリとアセット](/architecture/library-and-assets.md)
- [Luaシーン契約](/architecture/lua-scene-contract.md)

# Implementation Notes

法線取得とコピー不要の共有テクスチャハンドルだけをC++境界へ追加し、Cook-Torrance評価とアセット固有の構成はLuaに置く。
