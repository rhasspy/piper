## 概要

WindowsビルドでテストTTS実行時に発生するアクセス違反エラー（Exit code -1073741819 / 0xC0000005）を修正しました。

関連Issue: #51

## 問題の原因

調査の結果、MSVCランタイムライブラリの不一致が原因であることが判明しました：
- piper本体とその依存ライブラリ（fmt、spdlog）が静的ランタイム（/MT）でビルドされていた
- ONNX Runtimeは動的ランタイム（/MD）でビルドされている
- この不一致によりメモリ管理に問題が発生し、アクセス違反エラーが発生

## 変更内容

### 1. MSVCランタイムライブラリの統一
- CMakeLists.txtでMSVCランタイムライブラリを動的ランタイム（/MD、/MDd）に統一
- `CMAKE_MSVC_RUNTIME_LIBRARY`を`MultiThreadedDLL$<$<CONFIG:Debug>:Debug>`に設定
- すべての外部プロジェクト（fmt、spdlog、piper-phonemize）に同じ設定を適用

### 2. Windowsライブラリリンクの修正
- Debug/Releaseビルドで適切なライブラリをリンクするように修正
- fmt/spdlogのDebugビルドでは`fmtd.lib`/`spdlogd.lib`を使用

### 3. build-windows.ymlの改善
- Debug/Releaseビルドでそれぞれ適切なディレクトリを参照するように修正
- ONNX Runtimeのチェックタイミングを適切に調整

## テスト結果

- ✅ Windowsビルドが成功
- ✅ Linux/macOSビルドに影響なし
- 🔄 Windows実行時テストの確認中

## 今後の課題

- Windows実行時のDLL依存関係の最適化
- espeak-ng DLLのパス設定の改善
- OpenJTalkのWindows対応（将来的な実装）

## コミット履歴

- MSVCランタイムライブラリを動的リンクに変更
- fmt/spdlogライブラリにも同じランタイム設定を適用
- ExternalProjectでCMAKE_ARGSを正しく渡すように修正
- build-windows.ymlでDebug/Releaseディレクトリを正しく参照
EOF < /dev/null