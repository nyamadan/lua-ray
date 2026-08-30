---
type: Change Specification
title: GitHub Actionsの更新とSHA固定
description: CIで利用するGitHub Actionsを最新安定版へ更新し、完全長コミットSHAで固定する。
tags: [ci, github-actions, supply-chain]
status: stable
generated: { by: process:github-actions-upgrade, at: 2026-08-30T23:02:45Z }
verified:
  { by: process:github-actions-upgrade-verification, at: 2026-08-30T23:03:51Z }
sources:
  - id: ci
    resource: ../../.github/workflows/ci.yml
    title: CIワークフロー
  - id: workflow
    resource: /development/workflow.md
    title: 開発・検証ワークフロー
---

# Intent

CIで利用するGitHub Actionsを最新の安定版へ更新し、変更可能なタグ参照ではなく完全長コミットSHAへ固定することで、CIの再現性とサプライチェーン安全性を高める。

# Non-goals

- Action以外の依存、emsdk、Node.js、pnpm、および製品コードは更新しない。
- CIのジョブ構成、入力、成果物、ビルド・テスト処理は変更しない。

# Acceptance Criteria

- [x] `.github/workflows/ci.yml` の全 `uses:` が公式安定版の40文字コミットSHAへ固定される。
- [x] 各 `uses:` に対応するリリースバージョンが行末コメントで記録される。
- [x] CIワークフローがYAMLとして構文解析できる。
- [x] OKF仕様の構造、リンク、Markdown書式が検証に成功する。

# Affected Concepts

- [開発・検証ワークフロー](/development/workflow.md)

# Implementation Notes

公式リポジトリのリリースタグを `git ls-remote` で完全長コミットSHAへ解決する。注釈付きタグはタグオブジェクトではなく `^{}` の参照先コミットを採用する。Actionの既存入力は維持し、`uses:` の参照とバージョンコメントだけを変更する。

# Verification Result

checkout 7.0.1、cache 6.1.0、pnpm/action-setup 6.0.10、setup-node 7.0.0、upload-artifact 7.0.1を採用した。YAML解析、全 `uses:` の完全長SHA・バージョンコメント検査、公式タグとのSHA照合、OKF文書のPrettier検査、スキル構造検証はすべて成功した。
