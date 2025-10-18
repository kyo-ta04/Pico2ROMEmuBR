## 目的
このリポジトリは Raspberry Pi Pico2 (RP2350) を使った ROM エミュレータです。AI エージェントはここから短時間で実装・修正・デバッグができるように、プロジェクト固有の構造・ワークフロー・注意点をまとめます。

## 重要ポイント（短く）
- メインソース: `Pico2ROMEmuBR.c` — PIO（`rom_emu.pio`）を使って外部バス（A0..A12, D0..D7）を監視し、ROM データを返します。
- ROM データ: `rom_basic_const.c`（約8KB配列）を実行時に `rom_data`（RAM）に memcpy して使用します。配列は `hex2c.py` で生成します。
- PIO プログラム: `oe_address_control`, `clk_out`, `reset_out`（定義は `rom_emu.pio`）。PIO は pio0 の SM 0..2 を使用する実装になっています。
- マルチコア: core1 が ROM 応答ループを実行し、core0 が PIO と初期化・UART-USB ブリッジを担当します（`multicore` と FIFO でハンドシェイク）。

## 開発ワークフロー（必須のコマンド例）
- Configure + build (PowerShell、リポジトリルート):
```powershell
./scripts/build.ps1    # configure + build
./scripts/build.ps1 -Clean
./scripts/build.ps1 -NoConfigure  # 既に configure 済みの場合
```
- 生成物／フラッシュ（VS Code タスクが設定済み）:
  - タスク名 "Compile Project" — ninja ビルド
  - タスク名 "Run Project" — `picotool load <path> -fx` を実行
  - タスク名 "Flash" — openocd を使った書き込み
 これらはワークスペースの tasks 定義（VS Code 拡張環境）と `CMakeLists.txt` の `PICO_BOARD`／`${USERPROFILE}/.pico-sdk` を前提とします。

## ソースコードで覚えておくべきパターンとファイル
- `Pico2ROMEmuBR.c`
  - ピン割り当て定義: `DATA_PINS_BASE` (GP2), `ADDR_PINS_BASE` (GP10), `OE_PIN` (GP26), `CS_PIN` (GP27), `RESETOUT_PIN` (GP25), `CLKOUT_PIN` (GP28)
  - PIO 初期化: `pio_add_program` → `pio_sm_init` → `pio_sm_set_enabled`
  - core1 のループ: `core1_entry()` が `pio_sm_get_blocking()` でアドレスを受け取り、`pio_sm_put()` でデータを返す
  - ROM 初期化ルート: `init_rom_basic_code()` が `memcpy` で `rom_basic` を `rom_data` にコピー
- `rom_emu.pio`
  - アドレス読み取り (in pins,13)、データ出力 (out pins,8)、オープンコレクタ風の動作を PIO で実装
- `rom_basic_const.c` と `hex2c.py`
  - 新しい ROM イメージを反映する場合は、Intel HEX を `hex2c.py` に渡して `rom_basic_const.c` を再生成する（生成後、ビルド）。

## 作業時の注意・制約
- ROM データは小さめ（8192 バイト）と仮定している。配列はソースにべた書きされているため、巨大配列の編集は PR 差分が大きくなります。
- PIO とマルチコアの組合せでタイミング依存の不具合が出やすい。printf によるログは USB stdio（CMake で有効）で出力されるため、デバッグ時は `pico_enable_stdio_usb` の設定を確認する。
- ビルドツールはユーザーの `~/.pico-sdk` にある ninja / picotool / openocd を参照するスクリプト（`scripts/build.ps1`）を利用する想定。CI を追加する場合は `.pico-sdk` のパスと ninja バージョン管理に注意。

## よくある小さなタスクと AI 向けヒント
- 新しい ROM を差し替える:
  1) Intel HEX をプロジェクトルートに置く（例: `SAKI80MB.HEX`）
  2) `python hex2c.py` を実行して `rom_basic_const.c` を生成
  3) `./scripts/build.ps1` でビルド
- PIO を変更する:
  - `rom_emu.pio` を編集 → CMake が pio ヘッダを生成する（`pico_generate_pio_header` が CMakeLists にある） → ビルド
- デバッグ手順のヒント:
  - 起動ログは USB stdio（`stdio_init_all()` 後の printf）に出る。core1 の起動ハンドシェイクは `FLAG_VALUE` を使っているので、FIFO の push/pop の流れを確認する。

## 具体的なプロンプト例（AI に対して）
- "rom_basic_const.c を生成するための hex の変換手順を書いて。hex2c.py の使い方と注意点を示して。"
- "`rom_emu.pio` の `oe_address_control` を 1 クロック遅らせる変更案を提示し、必要な C 側のパラメータ変更を説明して。"

## 参照ファイル
- `Pico2ROMEmuBR.c`, `rom_emu.pio`, `rom_basic_const.c`, `hex2c.py`, `CMakeLists.txt`, `scripts/build.ps1`, `README.md`, `README.en.md`

---
フィードバックください：もっと詳しく欲しい箇所（例：PIO のタイミング解析、ROM 生成の自動化、CI 用のビルド定義）を教えてください。
