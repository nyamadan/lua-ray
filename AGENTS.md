# リポジトリガイドライン

## プロジェクト構造とモジュール構成

このプロジェクトは、C++/WebAssemblyの構造とNode.jsツールチェーンに従っています：

- `main.cpp`: SDL3を使用したコアアプリケーションのエントリーポイント
- `CMakeLists.txt`: C++アプリケーションのビルド設定
- `package.json`: pnpmによるNode.jsパッケージ管理
- `.emsdk/`: WebAssemblyコンパイル用のEmscripten SDK
- `build/`: CMakeビルド出力ディレクトリ

## ビルド、テスト、開発コマンド

- `cmake --build ./build/gcc-debug `: プロジェクトのビルド
- `ctest --test-dir ./build/gcc-debug --exclude-regex "^prim_"`
- `./build/gcc-debug/lua-ray`: コンパイルされたアプリケーションの実行
- `pnpm install`: Node.js依存関係のインストール

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
