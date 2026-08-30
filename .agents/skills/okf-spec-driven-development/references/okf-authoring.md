# OKF v0.2 執筆規約

この資料は、`specs/` を更新する際に [OKF v0.2仕様](https://github.com/GoogleCloudPlatform/knowledge-catalog/blob/main/okf/SPEC.md) から適用する最小規約である。

## 文書と束

- 束はMarkdownファイルのディレクトリツリーで、各非予約 `.md` はUTF-8、先頭の `---` で囲んだYAML frontmatter、Markdown本文の順にする。
- 各概念の `type` は空でない文字列を必須とする。`title`、`description`、`resource`、`tags`、任意のプロジェクト固有キーは必要な場合だけ使う。
- `index.md` はディレクトリ一覧、`log.md` は日付順の更新履歴であり、通常の概念として扱わない。ルート `index.md` だけ `okf_version: "0.2"` をfrontmatterに持てる。
- `/` で始まるリンクは束ルート相対、その他は通常の相対リンクとする。リンク切れは警告対象だが、OKFの不適合として概念を破棄しない。

## 根拠・信頼・鮮度

```yaml
generated: { by: process:initial-okf-specification, at: 2026-08-30T00:00:00Z }
sources:
  - id: source-id
    resource: ../src/example.cpp
    title: 根拠の表示名
verified:
  - { by: process:verification, at: 2026-08-30T00:00:00Z }
status: stable
stale_after: 2027-08-30T00:00:00Z
```

- `sources[].resource` は絶対URL、束ルート相対パス、または相対パス。`id` は本文脚注の安定した結合キーにする。
- `generated` は内容を書いた主体と時刻、`verified` は根拠との一致を確認した主体と時刻であり、混同しない。
- actorは `process:<id>`、`human:<id>`、`<producer>/<version>` のいずれか。人手確認は必ず `human:` で始める。
- `status` は `draft`、`stable`、`deprecated`。省略時のOKF既定は `stable` だが、調査中の新規変更仕様は明示的に `draft` にする。
- `stale_after` はUTCオフセット付きISO 8601日時で、現在時刻がその瞬間以上なら stale と判断する。

## 変更仕様の書き方

変更仕様は `type: Change Specification` とし、本文に次を含める。

1. `# Intent`: 誰のどの問題を解決するか。
2. `# Non-goals`: 今回扱わない範囲。
3. `# Acceptance Criteria`: 実行可能または観測可能な条件をチェックリストで書く。
4. `# Affected Concepts`: 既存概念へのリンク。
5. `# Implementation Notes`: 実装箇所とテストの対応。ここは設計を拘束する必要がある場合だけ記す。

受入条件は「正しい」「使いやすい」ではなく、テスト名、コマンド、出力、API戻り値、状態遷移など観測可能な表現にする。
