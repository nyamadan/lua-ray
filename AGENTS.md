# リポジトリガイドライン

## プロジェクト構造とモジュール構成

このプロジェクトは、C++/WebAssemblyの構造とNode.jsツールチェーンに従っています：

- `src/`: ソースコードディレクトリ
  - `main.cpp`: SDL3を使用したアプリケーションのエントリーポイント
  - `app.cpp`, `app.h`: アプリケーションのメインロジック
  - `lua_binding.cpp`, `lua_binding.h`: Lua統合とバインディング
  - `embree_wrapper.cpp`, `embree_wrapper.h`: Embreeレイトレーシングのラッパー
  - `imgui_lua_binding.cpp`, `imgui_lua_binding.h`: ImGuiのLuaバインディング
  - `context.h`: アプリケーションコンテキスト定義
- `test/`: テストファイルディレクトリ
  - `lua_binding_test.cpp`: Luaバインディングのユニットテスト
  - `imgui_test.cpp`: ImGui統合テスト
  - `lbuffer_test.cpp`, `sol2_test.cpp`: ライブラリ統合テスト
  - `embree_link_test.cpp`, `lua_link_test.cpp`: リンクテスト
- `CMakeLists.txt`: C++アプリケーションのビルド設定
- `CMakePresets.json`: CMakeプリセット定義
- `package.json`: pnpmによるNode.jsパッケージ管理
- `main.lua`: メインのLuaスクリプト
- `triangles.lua`: 色付き三角形のレンダリングデモスクリプト
- `.emsdk/`: WebAssemblyコンパイル用のEmscripten SDK
- `build/`: CMakeビルド出力ディレクトリ

## ビルド、テスト、開発コマンド

- `cmake --build --preset gcc-debug`: プロジェクトのビルド (GCC Debug)
- `cmake --build --preset win32-release`: プロジェクトのビルド (Win32 Release)
- `cmake --build --preset emscripten-release`: プロジェクトのビルド (Emscripten)
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
