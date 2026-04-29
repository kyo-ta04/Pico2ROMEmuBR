#include <pico.h> 
#include <stddef.h>
#include <stdint.h>

extern const uint8_t __in_flash() __attribute__((aligned(4))) bios[];
extern const size_t bios_size;
