# リポジトリガイドライン

## プロジェクト構造とモジュール構成

このプロジェクトは、C++/WebAssemblyの構造とNode.jsツールチェーンに従っています：

- `src/`: ソースコードディレクトリ
  - `main.cpp`: SDL3を使用したアプリケーションのエントリーポイント
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
  - `imgui_test.cpp`: ImGui統合テスト
  - `embree_link_test.cpp`: Embreeリンクのテスト
  - `lua_link_test.cpp`: Luaリンクのテスト
- `CMakeLists.txt`: C++アプリケーションのビルド設定
- `CMakePresets.json`: CMakeプリセット定義
- `package.json`: pnpmによるNode.jsパッケージ管理
- `main.lua`: メインのLuaスクリプト
- `.emsdk/`: WebAssemblyコンパイル用のEmscripten SDK
- `build/`: CMakeビルド出力ディレクトリ

## ビルド、テスト、開発コマンド

- `cmake --build --preset gcc-debug`: プロジェクトのビルド (GCC Debug)
- `cmake --build --preset emscripten-debug`: プロジェクトのビルド (Emscripten Debug)
- `ctest --preset gcc-debug --output-on-failure`: テストの実行
- `./build/gcc-debug/lua-ray`: コンパイルされたアプリケーションの実行
- `pnpm install`: Node.js依存関係のインストール

または、package.jsonのスクリプトを使用：

- `pnpm build`: プロジェクトのビルド (GCC)
- `pnpm build:win32`: プロジェクトのビルド (Win32)
- `pnpm build:emscripten`: プロジェクトのビルド (Emscripten)
- `pnpm test`: テストの実行

## コーディングスタイルと命名規則

- C++コードは標準的な規則に従い、インデントは4スペース
- 記述的でわかりやすい変数名と関数名をcamelCaseで使用
- SDL操作には適切なエラーハンドリングを含める

## テストガイドライン

- テストにはCTestフレームワークを使用
- テストはビルドディレクトリで`ctest`コマンドで実行

## コミットとプルリクエストのガイドライン

- コンベンショナルコミットメッセージを使用
- コミット説明に関連するイシューを参照
- PRを提出する前にコードが正常にコンパイルできることを確認
