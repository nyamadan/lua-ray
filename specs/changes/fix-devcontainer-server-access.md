---
type: Change Specification
title: DevContainer開発サーバーのホストアクセス修正
description: DevContainer内の開発サーバーへホストのブラウザからアクセス可能にする。
tags: [development, devcontainer, networking]
status: stable
generated: { by: process:devcontainer-server-access, at: 2026-08-30T23:12:31Z }
verified:
  {
    by: process:devcontainer-server-access-verification,
    at: 2026-08-30T23:14:05Z,
  }
sources:
  - id: server
    resource: ../../server-cross-origin.py
    title: クロスオリジン開発サーバー
  - id: devcontainer
    resource: ../../.devcontainer/devcontainer.json
    title: DevContainer設定
---

# Intent

DevContainer内で起動したクロスオリジン開発サーバーを、Dockerのpublished portを介してホストのブラウザから利用可能にする。

# Non-goals

- 開発サーバーをホストのloopback以外やLANへ公開しない。
- ポート番号、配信内容、CORS、COEP、COOPヘッダーは変更しない。
- `forwardPorts` は使用しない。

# Acceptance Criteria

- [x] 開発サーバーがコンテナ内の全IPv4インターフェースでポート8143を待ち受ける。
- [x] DevContainerが `appPort` でホストの `127.0.0.1:8143` をコンテナのポート8143へ公開する。
- [x] DevContainerがhost networkを使用しない。
- [x] loopbackおよびコンテナの非loopbackアドレスからHTTP応答を取得できる。
- [x] 既存のCORS、COEP、COOPレスポンスヘッダーが維持される。

# Affected Concepts

- [開発・検証ワークフロー](/development/workflow.md)

# Implementation Notes

開発サーバーの既定bindアドレスを `0.0.0.0` とし、ブラウザ向け案内URLは `localhost` のまま表示する。DevContainerでは `appPort` のloopback限定マッピングを維持し、`--network=host` を削除する。

# Verification Result

`ss` で `0.0.0.0:8143` の待受を確認し、コンテナ内のloopbackと非loopbackアドレスの両方からHTTP 200を取得した。両応答でCORS、COEP、COOPヘッダーが維持されることを確認した。ホスト側のpublished portはDevContainer再構築後に確認する。
