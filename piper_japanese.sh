#!/bin/bash
# Piper日本語音声合成 簡単実行スクリプト

# スクリプトのディレクトリを取得
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Piperのバイナリとデータのパスを設定
PIPER_BIN="${SCRIPT_DIR}/piper/bin/piper"
ESPEAK_DATA="${SCRIPT_DIR}/piper/espeak-ng-data"

# espeak-ngのデータパスを設定
export ESPEAK_DATA_PATH="${ESPEAK_DATA}"

# Piperが存在するかチェック
if [ ! -f "${PIPER_BIN}" ]; then
    echo "エラー: Piperが見つかりません: ${PIPER_BIN}"
    echo "まず、Piperをダウンロードして解凍してください："
    echo ""
    echo "  curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper.tar.gz"
    echo "  tar -xzf piper.tar.gz"
    echo "  xattr -cr piper/"
    echo ""
    exit 1
fi

# 引数をそのままPiperに渡す
"${PIPER_BIN}" "$@"