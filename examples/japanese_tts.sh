#!/bin/bash
# Piper日本語音声合成の使用例

# Piperの実行ファイルパス（環境に合わせて変更してください）
PIPER_BIN="./piper/bin/piper"

# 日本語モデルのパス（環境に合わせて変更してください）
MODEL_PATH="path/to/your/japanese_model.onnx"

# 基本的な使用例
echo "=== 基本的な使用例 ==="
echo "こんにちは、Piperです。" | $PIPER_BIN --model "$MODEL_PATH" --output_file hello.wav
echo "hello.wav を生成しました"

# 長いテキストの例
echo "=== 長いテキストの例 ==="
cat << EOF | $PIPER_BIN --model "$MODEL_PATH" --output_file long_text.wav
本日は晴天なり。
日本語の音声合成システムPiperを使用して、
様々なテキストを自然な音声に変換することができます。
OpenJTalkを統合することで、
正確な日本語の発音を実現しています。
EOF
echo "long_text.wav を生成しました"

# 複数の文を個別に処理
echo "=== バッチ処理の例 ==="
texts=(
    "おはようございます"
    "こんにちは"
    "こんばんは"
    "おやすみなさい"
)

for i in "${!texts[@]}"; do
    echo "${texts[$i]}" | $PIPER_BIN --model "$MODEL_PATH" --output_file "greeting_$i.wav"
    echo "greeting_$i.wav を生成しました"
done

# デバッグ情報を表示する例
echo "=== デバッグ情報の表示 ==="
echo "テスト" | $PIPER_BIN --model "$MODEL_PATH" --output_file test.wav --debug

echo "=== 完了 ==="
echo "生成された音声ファイル:"
ls -la *.wav