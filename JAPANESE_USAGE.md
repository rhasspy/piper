# Piper 日本語音声合成ガイド

このガイドでは、Piperを使用して日本語の音声合成を行う方法を説明します。

## 必要なもの

- macOS、Linux、またはWindows（※Windows版は現在開発中）
- 日本語対応のPiperモデル（.onnxファイル）

## インストール方法

### macOSの場合

1. 最新のリリースから、お使いのMacに対応したバイナリをダウンロードします：
   - Apple Silicon (M1/M2/M3)の場合: `piper_macos_aarch64.tar.gz`
   - Intel Macの場合: `piper_macos_x64.tar.gz`

   ```bash
   # Apple Siliconの例
   curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper_macos_aarch64.tar.gz
   ```

2. ダウンロードしたファイルを解凍します：
   ```bash
   tar -xzf piper_macos_aarch64.tar.gz
   ```

3. 解凍されたディレクトリ構造を確認します：
   ```
   piper/
   ├── bin/
   │   └── piper          # 実行ファイル
   ├── lib/               # 必要なライブラリ
   ├── share/
   │   └── piper/
   │       └── openjtalk-dict/  # 日本語辞書
   └── その他のファイル
   ```

### Linuxの場合

1. 最新のリリースから、お使いのアーキテクチャに対応したバイナリをダウンロードします：
   - AMD64: `piper_linux_amd64.tar.gz`
   - ARM64: `piper_linux_arm64.tar.gz`

   ```bash
   # AMD64の例
   curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_linux_amd64.tar.gz -o piper_linux_amd64.tar.gz
   tar -xzf piper_linux_amd64.tar.gz
   ```

## 日本語モデルの入手

日本語の音声合成を行うには、OpenJTalk形式の音素を使用するモデルが必要です。

### 現在利用可能なモデル

1. **テスト用モデル**（開発・検証用）
   - piper-plusリポジトリの`test/models/`ディレクトリに含まれています
   - ファイル名: `ja_JP-test-medium.onnx`と`ja_JP-test-medium.onnx.json`
   - GitHubからダウンロード:
     ```bash
     # ONNXモデル（約63MB）
     curl -L -o ja_JP-test-medium.onnx https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx
     
     # 設定ファイル
     curl -L -o ja_JP-test-medium.onnx.json https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx.json
     ```

2. **自作モデル**
   - [トレーニングガイド](TRAINING.md)を参照して独自のモデルを作成できます

### モデルの要件

モデルの設定ファイル（.onnx.json）に以下の設定が必要です：
```json
{
  "phoneme_type": "openjtalk",
  "language": {
    "code": "ja"
  }
}
```

## 使用方法

### 自動セットアップ（新機能）

最新バージョンでは、OpenJTalk辞書とHTSボイスモデルが**自動的にダウンロード**されます！

```bash
# espeak-ngのデータパスだけ設定すれば、すぐに使えます
export ESPEAK_DATA_PATH="$(pwd)/piper/share/espeak-ng-data"

# 日本語音声を生成（初回実行時に必要なファイルが自動ダウンロードされます）
echo "こんにちは" | ./piper/bin/piper --model path/to/model.onnx --output_file hello.wav
```

#### 自動ダウンロードの詳細

初回実行時に以下のファイルが自動的にダウンロードされます：
- **OpenJTalk辞書**: `~/.piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-1.11/`
- **HTSボイスモデル**: `~/.piper/voices/hts/hts_voice_nitech_jp_atr503_m001-1.05/`

#### 環境変数による制御

- `PIPER_AUTO_DOWNLOAD_DICT=0`: 自動ダウンロードを無効化
- `PIPER_OFFLINE_MODE=1`: オフラインモード（ダウンロードを試みない）
- `OPENJTALK_DICTIONARY_DIR`: カスタム辞書パスを指定
- `OPENJTALK_VOICE`: カスタムボイスモデルパスを指定

### 手動セットアップ（従来の方法）

自動ダウンロードを使用したくない場合や、カスタムファイルを使用する場合は、以下の手順で手動セットアップできます：

#### 1. 必要なファイルのダウンロード

```bash
# 作業ディレクトリを作成
mkdir -p ~/piper-japanese
cd ~/piper-japanese

# OpenJTalk辞書をダウンロード
curl -L -o open_jtalk_dic.tar.gz "https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.11/open_jtalk_dic_utf_8-1.11.tar.gz/download"
tar -xzf open_jtalk_dic.tar.gz

# HTSボイスモデルをダウンロード（音素抽出に必要）
curl -L -o hts_voice.tar.gz "https://sourceforge.net/projects/open-jtalk/files/HTS%20voice/hts_voice_nitech_jp_atr503_m001-1.05/hts_voice_nitech_jp_atr503_m001-1.05.tar.gz/download"
tar -xzf hts_voice.tar.gz
```

#### 2. 環境変数の設定

```bash
# espeak-ngのデータパス（Piperの初期化に必要）
export ESPEAK_DATA_PATH="$(pwd)/piper/share/espeak-ng-data"

# OpenJTalk辞書のパス（日本語の音素抽出に使用）
export OPENJTALK_DICTIONARY_DIR="$(pwd)/open_jtalk_dic_utf_8-1.11"

# HTSボイスモデルのパス（OpenJTalkの音素抽出に必要）
export OPENJTALK_VOICE="$(pwd)/hts_voice_nitech_jp_atr503_m001-1.05/nitech_jp_atr503_m001.htsvoice"
```

### 基本的な使い方

```bash
# テキストファイルから音声を生成
./piper/bin/piper --model path/to/model.onnx --output_file output.wav < input.txt

# 直接テキストを入力
echo "こんにちは、世界" | ./piper/bin/piper --model path/to/model.onnx --output_file hello.wav

# 標準出力に音声データを出力（他のプログラムにパイプ）
echo "おはようございます" | ./piper/bin/piper --model path/to/model.onnx --output_raw | aplay -r 22050 -f S16_LE -t raw -
```

### 完全な手順例（ゼロから始める場合）

#### 最新版（自動ダウンロード対応）の場合：

```bash
# 1. 作業ディレクトリを作成
mkdir -p ~/piper-japanese-setup
cd ~/piper-japanese-setup

# 2. Piperバイナリをダウンロード（Apple Silicon Macの例）
curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper.tar.gz
tar -xzf piper.tar.gz

# 3. 日本語モデルをダウンロード
mkdir -p models
cd models
curl -L -o ja_JP-test-medium.onnx https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx
curl -L -o ja_JP-test-medium.onnx.json https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx.json
cd ..

# 4. 環境変数を設定（espeak-ngのパスのみ必要）
export ESPEAK_DATA_PATH="$(pwd)/piper/share/espeak-ng-data"

# 5. 日本語音声を生成（辞書とボイスモデルは自動でダウンロードされます）
echo "こんにちは、音声合成のテストです" | ./piper/bin/piper --model models/ja_JP-test-medium.onnx --output_file test.wav

# 6. 生成された音声を再生（macOSの場合）
afplay test.wav
```

#### 従来版（手動セットアップ）の場合：

```bash
# 1. 作業ディレクトリを作成
mkdir -p ~/piper-japanese-setup
cd ~/piper-japanese-setup

# 2. Piperバイナリをダウンロード（Apple Silicon Macの例）
curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper.tar.gz
tar -xzf piper.tar.gz

# 3. OpenJTalk辞書をダウンロード
curl -L -o open_jtalk_dic.tar.gz "https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.11/open_jtalk_dic_utf_8-1.11.tar.gz/download"
tar -xzf open_jtalk_dic.tar.gz

# 4. HTSボイスモデルをダウンロード
curl -L -o hts_voice.tar.gz "https://sourceforge.net/projects/open-jtalk/files/HTS%20voice/hts_voice_nitech_jp_atr503_m001-1.05/hts_voice_nitech_jp_atr503_m001-1.05.tar.gz/download"
tar -xzf hts_voice.tar.gz

# 5. 日本語モデルをダウンロード
mkdir -p models
cd models
curl -L -o ja_JP-test-medium.onnx https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx
curl -L -o ja_JP-test-medium.onnx.json https://github.com/ayutaz/piper-plus/raw/master/test/models/ja_JP-test-medium.onnx.json
cd ..

# 6. 環境変数を設定
export ESPEAK_DATA_PATH="$(pwd)/piper/share/espeak-ng-data"
export OPENJTALK_DICTIONARY_DIR="$(pwd)/open_jtalk_dic_utf_8-1.11"
export OPENJTALK_VOICE="$(pwd)/hts_voice_nitech_jp_atr503_m001-1.05/nitech_jp_atr503_m001.htsvoice"

# 7. 日本語音声を生成
echo "こんにちは、音声合成のテストです" | ./piper/bin/piper --model models/ja_JP-test-medium.onnx --output_file test.wav

# 8. 生成された音声を再生（macOSの場合）
afplay test.wav
```

### コマンドラインオプション

- `--model <path>`: 使用するモデルファイル（.onnx）のパス
- `--output_file <path>`: 出力する音声ファイルのパス
- `--output_raw`: WAVヘッダーなしの生の音声データを出力
- `--debug`: デバッグ情報を表示

### 環境変数

辞書ファイルの場所をカスタマイズする場合：
```bash
export OPENJTALK_DICTIONARY_DIR=/path/to/custom/dictionary
./piper/bin/piper --model model.onnx --output_file output.wav < input.txt
```

## トラブルシューティング

### 「espeak-ng data path」エラー

```
Error processing file '/usr/share/espeak-ng-data/phontab': No such file or directory.
```

このエラーが出る場合は、環境変数を設定してください：
```bash
export ESPEAK_DATA_PATH="$(pwd)/piper/espeak-ng-data"
```

### 大量の「Missing phoneme」警告

```
[warning] Missing "ˈ" (\u02C8): 5 time(s)
[warning] Missing "ː" (\u02D0): 2 time(s)
```

**原因：日本語モデルに英語テキストを入力している**

日本語モデル（OpenJTalk形式）は日本語専用です。英語テキストを入力すると、espeak-ngのIPA音素が使用され、これらは日本語モデルに登録されていません。

**解決方法：日本語テキストを使用してください**
- ✅ 正しい: `echo "こんにちは" | ./piper/bin/piper ...`
- ❌ 誤り: `echo "Hello" | ./piper/bin/piper ...`

### 「OpenJTalk: Failed to initialize」エラー

このエラーは辞書ファイルが見つからない場合に発生します。以下を確認してください：

1. `piper/share/piper/openjtalk-dict/`ディレクトリが存在するか
2. 環境変数`OPENJTALK_DICTIONARY_DIR`が正しく設定されているか

### 音声が生成されない

- モデルファイルが日本語対応（OpenJTalk形式）であることを確認
- 入力テキストがUTF-8エンコーディングであることを確認

### macOSでの「開発元が検証できない」警告

初回実行時にこの警告が表示される場合：
1. システム環境設定 → セキュリティとプライバシー
2. 「このまま開く」をクリック

または、ターミナルで以下を実行：
```bash
xattr -d com.apple.quarantine piper/bin/piper
```

## サンプルスクリプト

### バッチ処理スクリプト

```bash
#!/bin/bash
# batch_tts.sh - 複数のテキストファイルを音声に変換

MODEL_PATH="path/to/your/model.onnx"
PIPER_BIN="./piper/bin/piper"

for txt_file in *.txt; do
    base_name="${txt_file%.txt}"
    echo "処理中: $txt_file -> ${base_name}.wav"
    $PIPER_BIN --model "$MODEL_PATH" --output_file "${base_name}.wav" < "$txt_file"
done
```

### Pythonからの使用例

```python
import subprocess
import os

def text_to_speech(text, model_path, output_path):
    """テキストを音声ファイルに変換"""
    piper_bin = "./piper/bin/piper"
    
    # Piperを実行
    process = subprocess.Popen(
        [piper_bin, "--model", model_path, "--output_file", output_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    stdout, stderr = process.communicate(input=text)
    
    if process.returncode != 0:
        print(f"エラー: {stderr}")
        return False
    
    return True

# 使用例
text_to_speech(
    "本日は晴天なり",
    "path/to/model.onnx",
    "output.wav"
)
```

## 技術詳細

### OpenJTalkについて

PiperはOpenJTalkを使用して日本語テキストを音素に変換します。OpenJTalkは以下の処理を行います：

1. 形態素解析（MeCab）
2. 読み付与
3. アクセント処理
4. フルコンテキストラベル生成

### 音素について

OpenJTalkが生成する音素は、モデルの学習時に単一文字に変換されます。例：
- "ch" → "c", "h"
- "ts" → "t", "s"
- "sh" → "s", "h"

これにより、より柔軟な音素マッピングが可能になっています。

## ライセンス

- Piper: MIT License
- OpenJTalk: Modified BSD license
- naist-jdic: Modified BSD license

## モデルの互換性について

現在の実装では、既存のモデルをそのまま使用できますが、音素マッピングに関する警告が表示される場合があります。
詳細は[音素マッピングについて](PHONEME_MAPPING.md)を参照してください。

## 追加リソース

- **クイックスタート**: より簡潔な手順は[日本語クイックスタートガイド](docs/quick_start_japanese.md)を参照
- **サンプルスクリプト**: `examples/test_japanese_tts.sh`に実行例があります
- **技術詳細**: 日本語音素マッピングについては[PHONEME_MAPPING.md](PHONEME_MAPPING.md)を参照

## 関連リンク

- [Piper公式リポジトリ](https://github.com/rhasspy/piper)
- [OpenJTalk](http://open-jtalk.sourceforge.net/)
- [音素マッピングについて](PHONEME_MAPPING.md)