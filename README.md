# Pico2ROMEmu (SuperAKI80 CP/M 64K RAMEmu)
![Pico2ROMEmuBR](./IMG/Pico2ROMEmuBR_RAMEmu_title.jpg)
このプロジェクトは [Pico2ROMEmu](https://github.com/kyo-ta04/Pico2ROMEmuBR) を改造し、SuperAKI80上で64KB RAM及びメモリマップドI/OのUART及びROM/RAMディスクエミュレーションで CP/M v2.2を動かすものです。

## 特徴
- Raspberry Pi Pico2 追加GPIO引出し無し（GPIO25のみ）、使用GPIOは26+1ピン。
- RAMは 64KB全て Pico2のエミュレーション（要Super AKI-80基板上のSRAM無効化とMRWR接続）。
- UART及びROM/RAMディスクエミュレーションは メモリマップドI/Oを使用。
- リセットは手動、（要リセットSW）

## 構成
- KiCad 回路・基板データはこちら -> [Pico2ROMEmu_PCB](https://github.com/kyo-ta04/Pico2ROMEmu_PCB)
- CP/M 関係は z80pack、Z80-MBC2、The Unofficial CP/M Web site 等が元です。
    - z80pack - [unix4fun](http://cpmarchives.classiccmp.org/cpm/mirrors/www.autometer.de/unix4fun/z80pack/index.html), 
    [github](https://github.com/udo-munk/z80pack)
    - Z80-MBC2 - [Hackaday.io](https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer),
    [github](https://github.com/z80-mbc2/Z80-MBC2)
    - The Unofficial CP/M Web site - [cpm.z80.de](http://www.cpm.z80.de/)
- Super AKI-80(Z80) バスエミュレーションは EMUZ80、 emuz80_pico2等を参考にさせていただいてます。
    - EMUZ80 - [github](https://github.com/jCW299/emuz80) 
    - [電脳伝説](https://vintagechips.wordpress.com/)
    - [Super AKI-80](https://github.com/retro75/superaki-80/)
    - [Super AKI-80の組み立て方(最少部品編)](https://note.com/quiet_duck4046/n/n32906e1dfb96?sub_rt=share_sb)

[Z80-MBC2](https://z80-mbc2.github.io/)、 及び Z80-MBC2 を参考にさせていただいてます。　参考: 、
[CP/M関係](https://www.retrocomputing.stackexchange.com/questions/15777/running-cpm-3-on-z80-processor-with-4mb-of-ram) 
 
 [EMUZ80_RP2040_PCB_Firmware AE-2040_CPMブランチ](https://github.com/kyo-ta04/EMUZ80_RP2040_PCB_Firmware/tree/AE-2040_CPM) を参考にさせていただいてます。　参考: [EMUZ80_RP2040_PCB_Firmware AE-2040_CPMブランチ](https://github.com/kyo-ta04/EMUZ80_RP2040_PCB_Firmware/tree/AE-2040_CPM) 

- RP2350 PIO ROMエミュレーション部分は @tendai22plus さんの [ROMエミュレーション](https://github.com/tendai22/emuz80_pico2/blob/main/doc/ROM_EMULATION.md) を参考にさせていただいてます。　参考: [emuz80_pico2](https://github.com/tendai22/emuz80_pico2) 

## 回路図・資料
- 組み立て方の資料(not.com) 
  - [Pico2ROMEmuの組み立て方](https://note.com/quiet_duck4046/n/n425d6b7e8d55?sub_rt=share_sb)
  - [Super AKI-80の組み立て方(最少部品編)](https://note.com/quiet_duck4046/n/n32906e1dfb96?sub_rt=share_sb)  
- ![Pico2ROMEmuBR_sch](./IMG/Pico2ROMEmuBR_RAMEmu_sch.jpg)
  - 上記は回路図の画像です。
- ![Pico2ROMEmuBR_RUN_002](./IMG/Pico2ROMEmuBR_CPM_RUN_003.png)
  - 上記は実行例画像です。
- ![Pico2ROMEmuBR_RUN](./IMG/Pico2ROMEmuBR_CPM_RUN.png)
  - 上記は実行例画像です。

## ライセンス
- 本プロジェクトのソースコードは MIT ライセンスです。
- ROMデータ部分は元サイトおよび改編元など、各々のライセンスを参照してください。
- CP/Mその他に関してはこちらをご覧ください [EMUZ80_RP2040_PCB_Firmware AE-2040_CPMブランチ](https://github.com/kyo-ta04/EMUZ80_RP2040_PCB_Firmware/tree/AE-2040_CPM)

## 免責事項
本ソフトウェアは現状のまま提供されます。いかなる損害についても作者は責任を負いません。

## 謝辞
- Grant Searle さん（[BASICサブセット版/CP/M machine 作者）](http://searle.x10host.com/index.html)）
- @vintagechip さん（[電脳伝説さん Super AKI-80用BASIC 作者](https://vintagechips.wordpress.com/)）
- @tendai22plus さん ([emuz80_pico2  作者](https://github.com/tendai22/emuz80_pico2))
- Raspberry Pi Pico SDK 開発者の皆様
- 本プロジェクトに関わる全ての方々
