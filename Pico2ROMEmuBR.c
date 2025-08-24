#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/pll.h"
#include "pico/multicore.h"
#include "rom_emu.pio.h"
#include "rom_basic_const.c" 
// #include "rom_mon_const.c" 

#define DATA_PINS_BASE 2    // GP2～GP9 (D0-D7 8bit)
#define ADDR_PINS_BASE 10   // GP10～GP22 (A0-A12 13bit)
// #define RESETOUT_PIN 25     // GP25 (リセット出力)
#define SIDES_PIN_BASE 25 // GP25 LED
#define SIDES_PIN_NUM 1   // Sidesetピンの数

#define OE_PIN 26           // GP26 Output Enable (OE#)
#define CS_PIN 27           // GP27 Chip Select (CS#)
#define CLKOUT_PIN 28       // GP28 (クロック出力)

// UART0の設定
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define FLAG_VALUE 123

#define ROM_SIZE 8192

// PIO初期化
PIO pio = pio0;
uint sm = 0;


uint8_t rom_data[ROM_SIZE] __attribute__((section(".data")));

// コア1のエントリポイント
// core1_entry()はPIOの状態マシンを実行し、ROMデータを送信する  
__attribute__((noinline)) void __time_critical_func(core1_entry)(void) {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();
    while (true) {
        uint32_t address = pio_sm_get_blocking(pio, sm);    // アドレスを取得
        uint32_t data = rom_data[address];                  // 13bitアドレスに対応
        pio_sm_put(pio, sm, data);                          // データを送信
    }
}


// rom_basic[]をrom_data[]にコピーする初期化ルーチン
void init_rom_basic_code(void) {
    // z80_binary[]の内容をrom_data[]の先頭にコピー
    memcpy(rom_data, rom_basic, sizeof(rom_basic));
    // 残りのrom[]を0xFFで埋める（8Kバイトまで）
    memset(rom_data + sizeof(rom_basic), 0xFF, ROM_SIZE - sizeof(rom_basic));
}


// // rom_mon[]をrom_data[]にコピーする初期化ルーチン
// void init_rom_mon_code(void) {
//    // z80_binary[]の内容をrom_data[]の先頭にコピー
//    memcpy(rom_data, rom_mon, sizeof(rom_mon));
//    // 残りのrom[]を0xFFで埋める（8Kバイトまで）
//    memset(rom_data + sizeof(rom_mon), 0xFF, ROM_SIZE - sizeof(rom_mon));
//}
// QSPIクロックを調整する関数
void set_qspi_clock_divider(uint32_t sys_clock_khz, uint32_t qspi_max_khz) {
    uint32_t divider = (sys_clock_khz + qspi_max_khz - 1) / qspi_max_khz;
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, sys_clock_khz * 1000, sys_clock_khz * 1000 / divider);
}

int main() {
    uint32_t sysclk = 320 * 1000;           // Pico2 システムクロック 280/320/360MHz 
    vreg_set_voltage(VREG_VOLTAGE_1_30);    // 電圧を1.3Vに設定
    sleep_ms(100);                          // 電圧安定のための待機
    set_sys_clock_khz(sysclk, true);        // 高速動作
    set_qspi_clock_divider(sysclk, 133000); // QSPIクロックを133MHz以下に

    stdio_init_all();
    setbuf(stdout, NULL);           // 標準出力のバッファリングを無効化 

    // UART0の初期化
    uart_init(UART_ID, BAUD_RATE);
    // UARTピンの設定（GPIO0=TX, GPIO1=RX）
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // PIO初期化
    uint offset = pio_add_program(pio, &oe_address_control_program);
    pio_sm_config c = oe_address_control_program_get_default_config(offset);

    uint sm1 = 1; // sm1を使用
    uint offset1 = pio_add_program(pio, &clk_out_program);
    pio_sm_config c1 = clk_out_program_get_default_config(offset1);
 
    uint sm2 = 2; // sm2を使用
    uint offset2 = pio_add_program(pio, &reset_out_program);
    pio_sm_config c2 = reset_out_program_get_default_config(offset2);

    // GP0-7：出力
    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio, DATA_PINS_BASE + i);
    }
    // GP8-22：入力(13ピン A0-A12)
    for (int i = 0; i < 13; i++) {
        pio_gpio_init(pio, ADDR_PINS_BASE + i);
    }
    
    pio_gpio_init(pio, SIDES_PIN_BASE); // DEBUG用ピン Sideset GP25(LED)

    pio_gpio_init(pio, OE_PIN); // OEピン(GP26)の初期化
    pio_gpio_init(pio, CS_PIN); // CSピン(GP27)の初期化
    pio_gpio_init(pio, CLKOUT_PIN); // CLK出力ピン(GP28)の初期化

    sm_config_set_in_pins(&c, ADDR_PINS_BASE);
    sm_config_set_out_pins(&c, DATA_PINS_BASE, 8);
    sm_config_set_sideset_pins(&c, SIDES_PIN_BASE); // Sidesetピンの設定（GP25）
    sm_config_set_jmp_pin(&c, OE_PIN); // GPIO26 OEをJMPピンとして設定
    


    pio_sm_set_consecutive_pindirs(pio, sm, DATA_PINS_BASE, 8, false); // 出力ピン初期化
    pio_sm_set_consecutive_pindirs(pio, sm, SIDES_PIN_BASE, SIDES_PIN_NUM, true); // 出力ピン初期化

    // シフトレジスタの設定
    sm_config_set_in_shift(&c, false, false, 0); // ISR（入力シフトレジスタ）のシフト方向
    sm_config_set_out_shift(&c, true, false, 0); // OSR（出力シフトレジスタ）のシフト方向

    // sm1 のクロックを設定 
    sm_config_set_set_pins(&c1, CLKOUT_PIN, 1); // GP28をクロック出力ピンとして設定
    pio_sm_set_consecutive_pindirs(pio, sm1, CLKOUT_PIN, 1, true); // CLKOUTピンの初期化

    sm_config_set_clkdiv(&c1, sysclk / 40000); //  40MHz : 20MHz(10MHz 9600bps)

     // sm2 のリセット出力を設定
//    sm_config_set_set_pins(&c2, RESETOUT_PIN, 1); // GP25をリセット出力ピンとして設定
//    pio_sm_set_consecutive_pindirs(pio, sm2, RESETOUT_PIN, 1, true); // リセット出力ピンの初期化 
//    sm_config_set_clkdiv(&c2, sysclk / 10); //  10kHz (リセット出力のクロック)
    
    // sm2 のリセット出力プログラムをロード
//    pio_sm_init(pio, sm2, offset2, &c2);
//    pio_sm_set_pins(pio, sm2, 1); // ピン値を1（Hi）に設定（set_pinsのベースからのビット値）
//    pio_sm_set_enabled(pio, sm2, true);

    // sm のROMエミュプログラムをロード
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    sleep_ms(1); // 1ms待機

    // sm1 のクロック出力プログラムをロード
    pio_sm_init(pio, sm1, offset1, &c1);
    pio_sm_set_enabled(pio, sm1, true);
    init_rom_basic_code(); // rom_basic_const.cから初期化
//    init_rom_mon_code(); // rom_mon_const.cから初期化
    sleep_ms(3000); // 3秒待機
    // [Enter]入力を待つ
    printf("\n[Enter] を押すとPico2 ROMエミュレータのテスト開始します...\n");
    while (true) {
        int c = getchar_timeout_us(100000); // 100msタイムアウト
        if (c == '\r') { // [Enter]（CR）が入力されたら開始
            printf("Pico2 ROMエミュレータのテスト開始...\n");
            break;
        }
    }
    printf("\nPico2 システムクロック(1.3V) - %dMHz\n", sysclk / 1000);
    printf("リセット出力状態 - ON\n");
    printf("クロック出力(20MHz) 10MHz:9600bps - ON\n");
    printf("ROMエミュレータ起動 - core1\n");
    multicore_launch_core1(core1_entry);
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE)
        printf("うーん、コア0では正しくありません!\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("コア0ではすべてうまくいきました!\n");
    }

   uint32_t tim = 1000; 
//   printf("リセット解除まで - %d ms\n", tim);  
//   pio_sm_put(pio, sm2, tim);

    // メインループ
    printf("UART-USBブリッジ動作開始...\n");

    while (true) {
        // UARTの受信データがあるかチェック
        if (uart_is_readable(UART_ID)) {
            int c = uart_getc(UART_ID);     // UARTから1文字受信
            putchar_raw(c);                 // 受信データをそのままUSBへ送信
        }
        // USBから受信データがあるかチェック
        int c;
        if ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
            uart_putc_raw(UART_ID, c); // UARTへデータ送信
        }
    }
    return 0;
}
