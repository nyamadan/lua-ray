# lua-ray開発のための推奨コマンド

## ビルドコマンド

- **プロジェクトのビルド (GCC)**:
  ```bash
  cmake --build --preset gcc-debug
  ```
  または:
  ```bash
  pnpm build
  ```
- **プロジェクトのビルド (Emscripten)**:
  ```bash
  cmake --build --preset emscripten-debug
  ```
  または:
  ```bash
  pnpm build:emscripten
  ```
- **アプリケーションの実行**:
  ```bash
  ./build/gcc-debug/lua-ray
  ```
- **テストの実行**:
  ```bash
  ctest --preset gcc-debug --output-on-failure
  ```
  または:
  ```bash
  pnpm test
  ```

## 依存関係管理

- **Node.js依存関係のインストール**:
  ```bash
  pnpm install
  ```

## ユーティリティコマンド

- **ファイルのリスト表示**:
  ```bash
  ls
  ```
- **ディレクトリの変更**:
  ```bash
  cd <directory>
  ```
- **ファイルの検索**:
  ```bash
  find . -name "filename"
  ```
- **ファイル内のパターン検索**:
  ```bash
  grep "pattern" <file>
  ```
- **Gitコマンド**:
  ```bash
  git status
  git add <file>
  git commit -m "message"
  git push
  ```

## テストとデバッグ

- **特定のテストの実行**:
  ```bash
  ctest --preset gcc-debug -R <test_name>
  # 例: ワーカー関連のテストのみ実行
  ctest --preset gcc-debug -R worker
  ```
- **デバッグビルド**:
  ```bash
  cmake --build --preset gcc-debug
  ```
- **リリースビルド**:
  ```bash
  cmake --build --preset gcc-release
  ```

## コードスタイルとフォーマット

- **インデント**: C++コードには4つのスペースを使用。
- **命名**: 変数と関数にはキャメルケースを使用。
- **エラー処理**: SDL操作の適切なエラー処理を確実に実施。

## バージョン管理

- **Gitステータスの確認**:
  ```bash
  git status
  ```
- **Gitへのファイル追加**:
  ```bash
  git add <file>
  ```
- **変更のコミット**:
  ```bash
  git commit -m "message"
  ```
- **変更のプッシュ**:
  ```bash
  git push
  ```

## システムコマンド

- **現在のディレクトリの確認**:
  ```bash
  pwd
  ```
- **ディレクトリ内容のリスト表示**:
  ```bash
  ls -la
  ```
- **ファイル/ディレクトリの削除**:
  ```bash
  rm -rf <file/directory>
  ```
- **ディレクトリの作成**:
  ```bash
  mkdir <directory>
  ```
