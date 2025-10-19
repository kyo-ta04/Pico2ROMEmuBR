# Pico2ROMEmu 32KB - Tom's SBC 
![Pico2ROMEmu 32KB - Tom's SBC](./IMG/Pico2ROMEmu_32KB_Toms01.jpg)
このプロジェクトは Raspberry Pi Pico2を使用した簡単で高速なROMエミュレータです。
[English Readme](./README.en.md)
## 特徴
- 標準のPico2ではGPIO数の制約によりROM容量が8KB（A0〜A12）に限られますが、本プロジェクトではPico2とRP2350A_V10 CoreBoardを改造して32KB（A0〜A14）に拡張しています。これにより、より大きなROMイメージのエミュレーションが可能です。
- WeAct StudioのRP2350A_V10 CoreBoardは Raspberry Pi Pico 2互換の低価格な開発ボード。USB Type-C接続や追加LED・リセット(RUN)SWを備え、GPIO全ピンを引き出せることによる拡張性などが特徴です。  
Github -> [WeAct Studio RP2350A_V10 CoreBoard](https://github.com/WeActStudio/WeActStudio.RP2350ACoreBoard/tree/main/RP2350A_V10)
- 詳しくは[Pico2ROMEmu](https://github.com/kyo-ta04/Pico2ROMEmuBR)をご覧ください。

## 構成
- `rom_basic_const.c`のROMデータは Grant Searle氏が作成したものであり
[CP/M on a breadboard](http://searle.x10host.com/cpm/index.html)の [z80sbcFiles.zip](http://searle.x10host.com/cpm/z80sbcFiles.zip) に含まれる ROM.HEX由来です。

## 回路図・資料
- 組み立て方の資料(not.com) 
  - [Pico2ROMEmuの組み立て方](https://note.com/quiet_duck4046/n/n425d6b7e8d55?sub_rt=share_sb)
  - [Super AKI-80の組み立て方(最少部品編)](https://note.com/quiet_duck4046/n/n32906e1dfb96?sub_rt=share_sb)  

- ![Pico2ROMEmuBR](./IMG/Pico2ROMEmu_Toms01.jpg)
  - 上記はTom's SBC実装例の画像です。
- ![Pico2RomEmu 32K sch](./IMG/Pico2ROMEmu32KB_sch.jpg)  
  - 上記は回路図の画像です。
- ![Pico2ROMEmu 32KB RUN](./IMG/Pico2ROMEmu_32KB_Toms_RUN_1.jpg)
  - 上記は実行例画像です。

## ライセンス
- 本プロジェクトのソースコードは MIT ライセンスです。
- ROMデータ部分は元サイトおよび改編元など、各々のライセンスを参照してください。

## 免責事項
本ソフトウェアは現状のまま提供されます。いかなる損害についても作者は責任を負いません。

## 謝辞
- Grant Searle氏 [Grant's pages](http://searle.x10host.com/index.html)
- x.comnなどにてアドバイス、イイね、RPしていただいた皆様。
- Raspberry Pi Pico SDK 開発者の皆様
- 本プロジェクトに関わる全ての方々
- [Pico2ROMEmu](https://github.com/kyo-ta04/Pico2ROMEmuBR)もご覧ください。
