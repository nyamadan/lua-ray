---
okf_version: "0.2"
---

# lua-ray 仕様束

lua-ray の現行実装と、仕様から変更を実装するための入口。

# Architecture

- [システム概要](/architecture/system.md) - C++/Lua/Embree/SDL/Emscriptenの責務と境界
- [ランタイム構成](/architecture/runtime.md) - メインループ、Lua State、リソース所有権
- [Luaシーン契約](/architecture/lua-scene-contract.md) - シーンコールバックとライフサイクル
- [Luaバインディング](/architecture/bindings.md) - C++からLuaへ公開される型・関数
- [レンダリングパイプライン](/architecture/rendering-pipeline.md) - ブロック、バッファ、ワーカー、ポストエフェクト
- [ライブラリとアセット](/architecture/library-and-assets.md) - Luaライブラリ、glTF、テクスチャ、シーン

# Development

- [開発ワークフロー](/development/workflow.md) - ビルド、テスト、仕様同期、完了条件

# Change specifications

- [変更仕様一覧](/changes/index.md) - 仕様から実装する変更の入口
