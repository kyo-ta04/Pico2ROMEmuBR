#!/usr/bin/env python3
"""
CP/M Disk Image to C Array Converter for EMUZ80 RP2040
- ドライブレターによる自動設定 (A-D, I)
- RP2040 Flash配置属性 (__in_flash) と 4バイトアライメントの付与
- GCC範囲初期化による 0xE5 パディング
"""

import argparse
import os
import sys

def main():
    parser = argparse.ArgumentParser(
        description="CP/MディスクイメージをRP2040用Cソースに変換します。"
    )
    parser.add_argument("input_file", help="入力ディスクイメージ (.dsk, .bin等)")
    parser.add_argument("-o", "--output", help="出力Cソースのパス")
    parser.add_argument("-d", "--drive", choices=['A', 'B', 'C', 'D', 'I'], default='A',
                        help="ドライブレターを選択 (A-D: IBM 256KB, I: 650KB)")
    parser.add_argument("--name", help="C配列の変数名 (省略時は rom_disk_[D])")

    args = parser.parse_args()

    # ドライブ設定の決定
    if args.drive in ['A', 'B', 'C', 'D']:
        # IBM 8" SD 標準 (128 * 26 * 77 = 256,256)
        content_size = 128 * 26 * 77
        full_size = 256 * 1024  # 262,144
        type_str = "IBM 8\" SD"
        calc_str = "(128 * 26 * 77 = 256,256 / 256 * 1024 = 262,144)"
    else:
        # Drive I: HI-TECH C用 (128 * 26 * 200 = 665,600)
        content_size = 128 * 26 * 200
        full_size = 325 * 2048  # 665,600 (200トラックちょうど)
        type_str = "650KB Custom"
        calc_str = "(128 * 26 * 200 = 665,600 / 325 * 2048 = 665,600)"

    var_name = args.name if args.name else f"rom_disk_{args.drive}"
    size_macro = f"ROM_DISK_{args.drive}_SIZE"

    # ファイル読み込み
    try:
        if not os.path.exists(args.input_file):
            print(f"エラー: ファイルが見つかりません: {args.input_file}", file=sys.stderr)
            sys.exit(1)
            
        with open(args.input_file, "rb") as f:
            data = f.read()
    except Exception as e:
        print(f"エラー: {e}", file=sys.stderr)
        sys.exit(1)

    actual_size = len(data)
    if actual_size > full_size:
        print(f"警告: 入力サイズ({actual_size})が想定サイズ({full_size})を超えています。切り捨てられます。", file=sys.stderr)
        data = data[:full_size]

    # Cコード生成
    output = []
    output.append(f'/* Generated from "{args.input_file}" for Drive {args.drive}: ({type_str}) */')
    output.append('#include <pico.h>')
    output.append('#include <stddef.h>')
    output.append('#include <stdint.h>')
    output.append('')
    output.append(f'#define {size_macro} ({full_size})  // {calc_str}')
    output.append(f'const size_t {var_name}_len = {size_macro};')
    output.append('')
    output.append(f'const uint8_t __in_flash() __attribute__((aligned(4))) {var_name}[{size_macro}] = {{')

    # データ部分 (16バイトずつ)
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_values = ", ".join(f"0x{b:02x}" for b in chunk)
        output.append(f'    {hex_values}, // 0x{i:05x}')

    # 0xE5 パディング (GCCの範囲初期化子 [start ... end] = value を使用)
    if actual_size < full_size:
        output.append(f'    [{actual_size} ... {size_macro}-1] = 0xe5')

    output.append('};\n')
#    output.append(f'const size_t {var_name}_len = {size_macro};')

    c_code = "\n".join(output)

    if args.output:
        try:
            with open(args.output, "w", encoding="utf-8") as f:
                f.write(c_code)
            print(f"Drive {args.drive} 変換完了: {args.output}")
        except Exception as e:
            print(f"書き込みエラー: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        print(c_code)

if __name__ == "__main__":
    main()
