# hex2c.py
rom_size = 0x2000  # 必要に応じてサイズ調整
rom = [0xFF] * rom_size

with open("SAKI80MB.HEX", "r") as fin:
    for line in fin:
        if not line.startswith(":"):
            continue
        length = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        rectype = int(line[7:9], 16)
        if rectype == 0:
            for i in range(length):
                rom[addr + i] = int(line[9 + i*2:11 + i*2], 16)
        elif rectype == 1:
            break

with open("rom_basic_const.c", "w") as fout:
    fout.write(f"const unsigned char rom_basic[{rom_size}] = {{\n")
    for i in range(0, rom_size, 16):
        fout.write("  " + ", ".join(f"0x{b:02X}" for b in rom[i:i+16]) + f", // 0x{i:04X}\n")
    fout.write("};\n")
