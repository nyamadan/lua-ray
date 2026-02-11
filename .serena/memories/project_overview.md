# プロジェクト概要：lua-ray

## 目的

`lua-ray`プロジェクトは、Embreeを使用したレイトレーシング機能とLuaスクリプティングを統合したC++/WebAssemblyアプリケーションです。ウィンドウ処理と入力処理にSDL3を活用し、ネイティブ環境とWebベース環境の両方に適しています。

## 技術スタック

- **C++17**: アプリケーションのコア言語。
- **SDL3**: ウィンドウ処理と入力処理に使用。
- **Lua 5.4.6**: 拡張性のための組み込みスクリプト言語。
- **Embree 4.4.0**: 高性能レイトレーシングライブラリ。
- **Emscripten**: Webデプロイを可能にするためのWebAssembly（WASM）へのコンパイル用。
- **CMake**: 依存関係とコンパイルを管理するためのビルドシステム。
- **Node.js/pnpm**: JavaScript依存関係とツールの管理用。
- **Google Test**: ユニットテスト用（WebAssemblyビルドでは除外）。

## プロジェクト構造

- `src/`: ソースコードディレクトリ
  - `main.cpp`: アプリケーションのエントリーポイント
  - `app.cpp`, `app.h`: アプリケーションのメインロジック
  - `app_data.h`: アプリケーションデータ構造定義
  - `lua_binding.cpp`, `lua_binding.h`: Lua統合とバインディング
  - `imgui_lua_binding.cpp`, `imgui_lua_binding.h`: ImGuiのLuaバインディング
  - `embree_wrapper.cpp`, `embree_wrapper.h`: Embreeレイトレーシングのラッパー
  - `thread_worker.cpp`, `thread_worker.h`: スレッドワーカーの実装
  - `context.h`: アプリケーションコンテキスト定義
- `workers/`: Luaワーカースクリプト
  - `ray_worker.lua`: レイトレーシング計算用ワーカー
  - `posteffect_worker.lua`: ポストエフェクト処理用ワーカー
  - `worker_utils.lua`: ワーカー用ユーティリティ関数
- `test/`: テストファイルディレクトリ
  - `lua_binding_test.cpp`: Luaバインディングのユニットテスト
  - `thread_worker_test.cpp`: スレッドワーカーのテスト
  - `worker_utils_test.cpp`: ワーカーユーティリティのテスト
  - `block_utils_test.cpp`: ブロック管理のテスト
  - `lbuffer_test.cpp`: lbufferライブラリのテスト
  - `sol2_test.cpp`: sol2統合のテスト
  - `embree_link_test.cpp`: Embreeリンクのテスト
  - `lua_link_test.cpp`: Luaリンクのテスト
- `CMakeLists.txt`: CMakeのビルド設定
- `CMakePresets.json`: CMakeプリセット定義
- `package.json`: Node.jsパッケージ管理
- `.emsdk/`: WebAssemblyコンパイル用のEmscripten SDK
- `build/`: CMakeビルドの出力ディレクトリ

## 主な機能

- **クロスプラットフォーム**: WebAssembly経由でネイティブとWebデプロイの両方をサポート。
- **スクリプティング**: ランタイムスクリプティングと拡張性のための組み込みLua。
- **レイトレーシング**: 高性能レイトレーシングのためにEmbreeを使用。
- **テスト**: ユニットテスト用のGoogle Testフレームワーク（ネイティブビルドのみ）。

## ビルドと開発コマンド

- **ビルド (GCC)**: `cmake --build --preset gcc-debug`
- **ビルド (Emscripten)**: `cmake --build --preset emscripten-debug`
- **実行**: `./build/gcc-debug/lua-ray`
- **テスト**: `ctest --preset gcc-debug --output-on-failure`
- **依存関係のインストール**: `pnpm install`

または、package.jsonのスクリプトを使用：

- **ビルド**: `pnpm build`
- **ビルド (Emscripten)**: `pnpm build:emscripten`
- **テスト**: `pnpm test`

## コーディングスタイル

- **インデント**: 4つのスペース。
- **命名**: キャメルケースで説明的で明確な変数名と関数名。
- **エラー処理**: SDL操作の適切なエラー処理。

## テストガイドライン

- **フレームワーク**: Google TestとCTest。
- **実行**: ビルドディレクトリで`ctest`を使用してテストを実行。

## コミットガイドライン

- 従来型のコミットメッセージを使用。
- コミット説明で関連する問題を参照。
- PRを提出する前にコードが正常にコンパイルされることを確認。
