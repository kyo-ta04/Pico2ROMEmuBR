# Pico2ROMEmu
![Pico2ROMEmuBR](./IMG/Pico2ROMEmuBR_title_img.jpg)
このプロジェクトは Raspberry Pi Pico2を使用した簡単で高速なROMエミュレータです(バイナリはSuper AKI-80用) 

## 特徴
- 主要部品はPico2だけ、そして高速(Z80 10MHzノーウェイト！)
- Raspberry Pi Pico2 (RP2350) 専用です、 Pico (RP2040) ダメ。　ゼッタイ。
- Pico2 (電源3.3v) と Super AKI-80 (Z80ベースの SBC、電源5v)を直結するヤバいヤツです。（意味が解らない人は詳しい人に聞いてね）
- ROMエミュレーションは8kByte、ROM-BASIC。　他にリセット出力(オープンコレクタ)、クロック出力 20MHz (TMPZ84C015: Z80 10MHz動作)、UART-USB変換、電源供給(5V)機能あり。
- Super AKI-80は最小限の部品＆改造なし、これ一つだけで動作させることができます。
- RP2350のPIO、マルチコア、UART-USB変換、GPIO等のトレーニングを兼ねた実験的なプロジェクトです。

## 構成
- `rom_basic_const.c` などの ROM-BASIC 部分は [saki80basic](https://vintagechips.wordpress.com/2025/04/24/saki80basic/) 由来です。
  - 元の[BASICサブセット](http://searle.x10host.com/cpm/index.html)は Grant Searle 氏が作成したものであり、Super AKI-80用に @vintagechip（電脳伝説）さんが移植・改良されています。
  - 詳細は各々のライセンスを参照してください。
- RP2350 PIO ROMエミュレーション部分は @tendai22plus さんの [ROMエミュレーション](https://github.com/tendai22/emuz80_pico2/blob/main/doc/ROM_EMULATION.md) を参考にさせていただいてます(と言うかほぼいっしょ💦)

## 回路図・資料
- ![Pico2ROMEmuBR_img](./IMG/Pico2ROMEmuBR_img.jpg)
  - 上記はブレッドボードの実装例画像です。
- ![Pico2ROMEmuBR_sch](./IMG/Pico2ROMEmuBR_sch.png)
  - 上記は回路図の画像です。
- ![Pico2ROMEmuBR_RUN_img](./IMG/Pico2ROMEmuBR_RUN_img.png)
  - 上記は実行例画像です。

## ライセンス
- 本プロジェクトのソースコードは MIT ライセンスです。
- ROM-BASIC 部分は元サイトおよび改編元のライセンスを参照してください。

## 免責事項
本ソフトウェアは現状のまま提供されます。いかなる損害についても作者は責任を負いません。

## 謝辞
- Grant Searle 氏（BASICサブセット版作者）
- @vintagechip（電脳伝説）さん（Super AKI-80用BASIC作者）
- @tendai22plus さん ([emuz80_pico2](https://github.com/tendai22/emuz80_pico2) 作者)
- @shippoiincho さん、 @kondo_pc88 さん、 @TororoLab さん、 @I_HaL さん、 @antarcticlion さん、 @GAPUX さん、 @Tanuki_Bayashin さん、 @applesorce さん、@W88DodPECuThLOl さんを始めとしたアドバイス、イイね、RPしていただいた皆様。
- Raspberry Pi Pico SDK 開発者の皆様
- 本プロジェクトに関わる全ての方々

---

MIT License

Copyright (c) 2025 kyo-ta04(DragonballEZ)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
