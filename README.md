# lua-ray

**lua-ray** は、C++ と Lua を組み合わせた実験的なレイトレーシングレンダラーです。Embree をコアのレイトレーシングエンジンとして使用し、WebAssembly (Emscripten) を介してブラウザ上で動作させることも可能です。

## 目次

- [特徴](#特徴)
- [技術スタック](#技術スタック)
- [プロジェクト構造](#プロジェクト構造)
- [前提条件](#前提条件)
- [ビルドと実行](#ビルドと実行)
  - [ネイティブ (Windows/Linux/macOS)](#ネイティブ-windowslinuxmacos)
  - [WebAssembly (Web)](#webassembly-web)
- [テスト](#テスト)
- [ライセンス](#ライセンス)

## 特徴

- **ハイブリッドアーキテクチャ**: コアロジックは C++17 で記述され、柔軟なスクリプティングのために Lua 5.4 を組み込んでいます。
- **高性能レイトレーシング**: Intel Embree 4.4.0 を使用した高速なレイトレーシング。
- **クロスプラットフォーム**: SDL3 を使用し、ネイティブデスクトップアプリとしても、Webブラウザ上(WASM)でも動作します。
- **マルチスレッドレンダリング**: ワーカースレッド（WebではWeb Workers）を使用した並列レンダリング。
- **ライブコーディング**: Luaスクリプトを編集することで、再コンパイルなしにシーンやロジックを変更可能。

## 技術スタック

- **言語**: C++17, Lua 5.4, JavaScript/TypeScript
- **ライブラリ**:
  - [SDL3](https://github.com/libsdl-org/SDL): ウィンドウ管理、入力処理
  - [Embree](https://github.com/embree/embree): レイトレーシングカーネル
  - [ImGui](https://github.com/ocornut/imgui): GUI
  - [sol2](https://github.com/ThePhD/sol2): C++とLuaのバインディング
- **ビルドツール**: CMake, Emscripten, pnpm

## プロジェクト構造

```
lua-ray/
├── src/                # C++ ソースコード
├── workers/            # Lua ワーカースクリプト (レイトレーシング計算など)
├── test/               # ユニットテスト (Google Test)
├── cmake/              # CMake モジュール
├── build/              # ビルド出力ディレクトリ
├── CMakeLists.txt      # CMake 設定
├── CMakePresets.json   # CMake プリセット
├── package.json        # Node.js 依存関係 (Emscripten ビルド用ツールなど)
└── main.lua            # メインのエントリーポイントスクリプト
```

## 前提条件

- **C++ コンパイラ**: GCC, Clang, または MSVC (C++17 対応)
- **CMake**: 3.20 以上
- **Node.js**: v18 以上 (WebAssembly ビルドおよび依存管理用)
- **pnpm**: パッケージマネージャー

## ビルドと実行

依存関係のインストール:
```bash
pnpm install
```

### ネイティブ (Windows/Linux/macOS)

GCC/Clang (Debug):
```bash
cmake --build --preset gcc-debug
./build/gcc-debug/lua-ray
```

Windows (MSVC):
```bash
cmake --build --preset win32-release
```

### WebAssembly (Web)

Emscripten 環境が必要です（`emsdk` がインストールされているか、DevContainer を使用してください）。

```bash
cmake --build --preset emscripten-debug
# または
pnpm build:emscripten
```

ローカルサーバーで実行:
```bash
pnpm start
# ブラウザで http://localhost:8080 を開く
```

## テスト

Google Test を使用した C++ のユニットテストを実行します。

```bash
ctest --preset gcc-debug --output-on-failure
# または
pnpm test
```

特定のテストのみ実行する場合:
```bash
ctest --preset gcc-debug -R worker  # "worker" を含むテストを実行
```

## ライセンス

このプロジェクトは [MIT License](LICENSE) の下で公開されています。
