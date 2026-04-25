/* cpm22_1.h - Generated from "cpm22-1.dsk" for Drive A: (IBM 8" SD) */

#include <pico.h>
#include <stddef.h>
#include <stdint.h>

#define ROM_DISK_A_SIZE (262144)  // (128 * 26 * 77 = 256,256 / 256 * 1024 = 262,144)
extern const size_t cpm22_1_len;
extern const uint8_t __in_flash() __attribute__((aligned(4))) cpm22_1[ROM_DISK_A_SIZE];
