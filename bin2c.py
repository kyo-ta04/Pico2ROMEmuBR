#!/usr/bin/env python3
"""
バイナリファイルをC言語の配列ソースに変換するスクリプト (RP2040 / Pico 向け 4byteアライメント対応版)
"""

import argparse
import sys
import os

def main():
    parser = argparse.ArgumentParser(
        description="バイナリファイルをC言語の配列ソースに変換します。"
    )
    parser.add_argument("input_file", help="入力バイナリファイルのパス")
    parser.add_argument("-o", "--output", help="出力Cソースファイルのパス（省略時は標準出力）")
    parser.add_argument("--offset", type=lambda x: int(x, 0), default=0,
                        help="アドレスオフセット（例: 0x1000）")
    parser.add_argument("--name", default="data", help="C配列の変数名")
    parser.add_argument("--type", choices=["unsigned char", "uint8_t"],
                        default="uint8_t", help="配列の型")

    args = parser.parse_args()

    # ファイル読み込み
    try:
        with open(args.input_file, "rb") as f:
            raw_data = bytearray(f.read())
    except Exception as e:
        print(f"エラー: {e}", file=sys.stderr)
        sys.exit(1)

    original_len = len(raw_data)
    
    # 4バイトアライメントのためのパディング計算
    # (original_len + 3) & ~3 は、次の4の倍数に切り上げる計算
    aligned_len = (original_len + 3) & ~3
    padding_needed = aligned_len - original_len
    
    if padding_needed > 0:
        # 足りない分を 0x00 で埋める
        raw_data.extend([0x00] * padding_needed)

    # 16バイトずつ処理
    output_lines = []
    for i in range(0, len(raw_data), 16):
        chunk = raw_data[i:i+16]
        hex_str = ", ".join(f"0x{byte:02X}" for byte in chunk)
        addr = i + args.offset
        line = f"    {hex_str}, // 0x{addr:04X}"
        output_lines.append(line)

    # includeの決定
    includes = [
        '#include <pico.h>',
        '#include <stddef.h>'
    ]
    if args.type == "uint8_t":
        includes.append('#include <stdint.h>')
    
    include_section = "\n".join(includes)

    # Cコードを組み立て
    header = f"""/* Generated from "{args.input_file}" by bin2c.py
 * 元のサイズ: {original_len} バイト
 * アライン後サイズ: {aligned_len} バイト (4byte境界)
 * アドレスオフセット: 0x{args.offset:X}
 */

{include_section}

const size_t {args.name}_size = {aligned_len};
const {args.type} __in_flash() __attribute__((aligned(4))) {args.name}[] = {{
"""

    footer = """
};
"""

    c_code = header + "\n".join(output_lines) + footer

    # 出力
    if args.output:
        try:
            with open(args.output, "w", encoding="utf-8") as f:
                f.write(c_code)
            print(f"変換完了！({original_len} -> {aligned_len} bytes) → {args.output}")
        except Exception as e:
            print(f"書き込みエラー: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        print(c_code)


if __name__ == "__main__":
    main()
