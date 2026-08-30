# Directory Update Log

## 2026-08-30

- **Refactoring**: RayTracerの描画・ポストエフェクトに共通するworker、完了判定、コルーチン処理をRenderStageへ集約し、専用テストと既存テストで挙動を検証した。
- **Toolchain**: emsdkとEmscriptenを6.0.5へ更新し、Debug/ReleaseのWebAssembly build、GCC build、CTestで検証した。
- **Dependencies**: SDL 3.4.14、Lua 5.4.9、ImGui 1.92.9b、Embree 4.4.1、GoogleTest 1.18.0へ更新し、GCC、CTest、Emscripten、Win32で検証した。
- **Creation**: `lua-ray` の実装、テスト、README、CMake設定、既存の開発補助資料を根拠にOKF v0.2仕様束を作成した。
- **Creation**: OKF仕様から受入条件、実装、テスト、仕様同期までを扱うCodexスキルを追加した。
