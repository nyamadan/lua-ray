# Directory Update Log

## 2026-08-30

- **Rendering**: DamagedHelmetへ決定的RNG、GGX/Lambert BSDF、矩形面光源のNEE/MIS、linear累積を使う128 sppのprogressive path tracingシーンを追加した。
- **Rendering**: DamagedHelmet GLBの全PBRテクスチャ、Cook-Torrance BRDF、直接照明、影、ACESトーンマッピングを使うLuaシーンを追加した。
- **Rendering**: DamagedHelmetの初期カメラを正面から右上の三分の四ビューへ調整した。
- **Development**: DevContainerのhost networkを廃止し、loopback限定のappPortを介して開発サーバーへアクセスできる構成に変更した。
- **CI**: GitHub Actionsを最新安定版へ更新し、全 `uses:` を公式リリースタグに対応する完全長コミットSHAで固定した。
- **Refactoring**: RayTracerの描画・ポストエフェクトに共通するworker、完了判定、コルーチン処理をRenderStageへ集約し、専用テストと既存テストで挙動を検証した。
- **Toolchain**: emsdkとEmscriptenを6.0.5へ更新し、Debug/ReleaseのWebAssembly build、GCC build、CTestで検証した。
- **Dependencies**: SDL 3.4.14、Lua 5.4.9、ImGui 1.92.9b、Embree 4.4.1、GoogleTest 1.18.0へ更新し、GCC、CTest、Emscripten、Win32で検証した。
- **Creation**: `lua-ray` の実装、テスト、README、CMake設定、既存の開発補助資料を根拠にOKF v0.2仕様束を作成した。
- **Creation**: OKF仕様から受入条件、実装、テスト、仕様同期までを扱うCodexスキルを追加した。
