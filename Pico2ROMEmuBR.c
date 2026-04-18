#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/pll.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "tusb.h" // TinyUSBのヘッダーを追加

#include "rom_emu.pio.h"
// #include "rom_basic_const.c" 
#include "emubasic.c" 

#define A13_PIN 0           // GP0 A13 アドレス
#define A14_PIN 1           // GP1 A14 アドレス
#define DATA_PINS_BASE 2    // GP2～GP9 (D0-D7 8bit)
#define ADDR_PINS_BASE 10   // GP10～GP22 (A0-A12 13bit)
// #define RESETOUT_PIN 25     // GP25 (リセット出力)
#define MRWR_PIN 25         // GP25  メモリライト (MRWR#)
// #define OE_PIN 26           // GP26 Output Enable (OE#)
// #define CS_PIN 27           // GP27 Chip Select (CS#)
#define MRRD_PIN 26         // GP26 メモリリード (MRRD#)
#define A15_PIN 27          // GP27 A15 アドレス (A15)
#define CLKOUT_PIN 28       // GP28 (クロック出力)

// // UART0の設定
// #define UART_ID uart0
// #define BAUD_RATE 9600 // AKI-80 10MHz 9,600bps
// #define BAUD_RATE 4800  // AKI-80 5MHz 4,800bps
// #define BAUD_RATE 2400  // AKI-80 2.5MHz 2,400bps
// #define UART_TX_PIN 0
// #define UART_RX_PIN 1


#define FLAG_VALUE 123

// // 8192 = 0-0x1FFF
// #define MEMORY_SIZE 8192

// 65536 = 0-0xFFFF
#define MEMORY_SIZE 65536

// PIO初期化
PIO pio = pio0;
// uint sm = 0;
uint sm_trg = 0;
uint sm_emu = 1;


// 現在設定されているクロック周波数
uint32_t current_clk_freq = 0;

// リセット出力関連
volatile bool reset_released = false;

static uint8_t test_data[] = { // test program for debug
 0x3E, 0x00,       // LD A, 0x00
 0x32, 0x10, 0x00, // LD (0x0010), A
 0x3A, 0x10, 0x00, // LD A, (0x0010)
 0x3C,             // INC A
 0xC3, 0x02, 0x00  // JP 0x0002
};

static uint8_t test0_data[] = { // test program for debug
 0x3E, 0x41,       // LD A, 0x41
 0x32, 0x00, 0x1F, // LD (0x1F00), A
 0x76               // HALT
};

const uint8_t __in_flash() __attribute__((aligned(4))) test1_data[] = {
    0x21, 0x10, 0x00, 0x7E, 0xB7, 0xCA, 0x0F, 0x00, 0x32, 0x00, 0x1F, 0x23, 0xC3, 0x03, 0x00, 0x76, // 0x0000
    0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x21, 0x0D, 0x0A, 0x00, // 0x0010
    0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, // 0x0020
};

const uint8_t __in_flash() __attribute__((aligned(4))) test2_data[] = {
    0xF3, 0x31, 0x00, 0x20, 0x21, 0x1E, 0x00, 0x7E, 0xB7, 0xCA, 0x1D, 0x00, 0xF5, 0x3A, 0x01, 0x1F, // 0x0000
    0xCB, 0x4F, 0xCA, 0x0D, 0x00, 0xF1, 0x32, 0x00, 0x1F, 0x23, 0xC3, 0x07, 0x00, 0x76, 0x48, 0x65, // 0x0010
    0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x21, 0x0D, 0x0A, 0x00, 0x1A, 0x1A, // 0x0020
};


//const uint8_t __in_flash() __attribute__((aligned(4))) test3_data[] = {
//    0xF3, 0x31, 0x00, 0x20, 0x21, 0x33, 0x00, 0x7E, 0xB7, 0xCA, 0x13, 0x00, 0xCD, 0x25, 0x00, 0x23, // 0x0000 LD SP,2000H
//    0xC3, 0x07, 0x00, 0x3A, 0x01, 0x1F, 0xCB, 0x47, 0xCA, 0x13, 0x00, 0x3A, 0x00, 0x1F, 0xCD, 0x25, // 0x0010
//    0x00, 0xC3, 0x13, 0x00, 0x76, 0xF5, 0x3A, 0x01, 0x1F, 0xCB, 0x4F, 0xCA, 0x26, 0x00, 0xF1, 0x32, // 0x0020
//    0x00, 0x1F, 0xC9, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x21, // 0x0030
//    0x0D, 0x0A, 0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, // 0x0040
//};

const uint8_t __in_flash() __attribute__((aligned(4))) test3_data[] = {
    0xF3, 0x31, 0xED, 0x80, 0x21, 0x33, 0x00, 0x7E, 0xB7, 0xCA, 0x13, 0x00, 0xCD, 0x25, 0x00, 0x23, // 0x0000
    0xC3, 0x07, 0x00, 0x3A, 0x01, 0xE0, 0xCB, 0x47, 0xCA, 0x13, 0x00, 0x3A, 0x00, 0xE0, 0xCD, 0x25, // 0x0010
    0x00, 0xC3, 0x13, 0x00, 0x76, 0xF5, 0x3A, 0x01, 0xE0, 0xCB, 0x4F, 0xCA, 0x26, 0x00, 0xF1, 0x32, // 0x0020
    0x00, 0xE0, 0xC9, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x21, // 0x0030
    0x0D, 0x0A, 0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, // 0x0040
};


// Super AKI-80(TMPZ84C015) メモリ(RAM)エミュレーション用
// static uint8_t __attribute__((aligned(4))) memory[MEMORY_SIZE]  = {[0 ... MEMORY_SIZE - 1] = 0xFF};
static uint8_t __attribute__((aligned(4))) memory[MEMORY_SIZE];


// rom_basic[]をrom_data[]にコピーする初期化ルーチン
void init_rom_basic_code(void) {
//    memcpy(memory, test3_data, sizeof(test3_data));
//    memset(memory + sizeof(test3_data), 0xFF, 32768 - sizeof(test3_data));
//    memset(memory + 0x8000, 0, 32768);

    // z80_binary[]の内容をrom_data[]の先頭にコピー
//    memcpy(memory, rom_basic, sizeof(rom_basic));
    // 残りのmemory[]を0xFFで埋める
//    memset(memory + sizeof(rom_basic), 0xFF, MEMORY_SIZE - sizeof(rom_basic));
//    memset(memory + sizeof(rom_basic), 0xFF, 32768 - sizeof(rom_basic));
//    memset(memory + 0x8000, 0, 32768)

    memcpy(memory, emubasic, sizeof(emubasic));
    memset(memory + sizeof(emubasic), 0xFF, 32768 - sizeof(emubasic));
    memset(memory + 0x8000, 0, 32768);

}


// リセット解除タイマーコールバック
int64_t reset_timer_callback(alarm_id_t id, void *user_data) {
//    gpio_put(RESETOUT_PIN, 0);  // GP25をLow（リセット解除）
    reset_released = true;
    printf("リセット解除実行\n");
    return 0;
}


// QSPIクロックを調整する関数
void set_qspi_clock_divider(uint32_t sys_clock_khz, uint32_t qspi_max_khz) {
    uint32_t divider = (sys_clock_khz + qspi_max_khz - 1) / qspi_max_khz;
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, sys_clock_khz * 1000, sys_clock_khz * 1000 / divider);
}

// PWM初期化関数（表示周波数の2倍を出力）
void init_clk_pwm(uint32_t display_freq_hz) {
//    printf("クロック出力(PWM) - 設定中... 表示周波数: %u Hz\n", display_freq_hz);
    uint slice_num = pwm_gpio_to_slice_num(CLKOUT_PIN);
    uint channel = pwm_gpio_to_channel(CLKOUT_PIN);
    
    gpio_set_function(CLKOUT_PIN, GPIO_FUNC_PWM);
    
    // 表示周波数の2倍でPWMを設定
    uint32_t actual_freq_hz = display_freq_hz * 2;
    
    // システムクロックを取得（ハードコードを避ける）
    uint32_t sys_clock = clock_get_hz(clk_sys);
    printf("システムクロック: %u Hz\n", sys_clock);
    
    // PWM周波数 = sys_clock / ((wrap + 1) * clock_div)
    // 目標: PWM周波数 = actual_freq_hz
    float total_div = (float)sys_clock / actual_freq_hz;
//    printf("目標周波数: %u Hz, 必要な総分周比: %.2f\n", actual_freq_hz, total_div);
    
    // clock_div を 1.0 から 256.0 の範囲で最適化
    float clock_div = 1.0f;
//    uint16_t wrap = (uint16_t)(total_div / clock_div) - 1;
    uint32_t temp = (total_div / clock_div) - 1;
    
    if (temp > 65535) {
        // wrap が最大値を超える場合、clock_div を増やす
        clock_div = total_div / 65536.0f;
        if (clock_div > 256.0f) {
            clock_div = 256.0f;
            temp = (total_div / clock_div) - 1;
            if (temp > 65535) {
                temp = 65535; // 最低周波数に設定
            }
        } else {
            temp = 65535;
        }
    }
    uint16_t wrap = (uint16_t)temp;
//    printf("計算結果: total_div=%.2f, clock_div=%.2f, wrap=%u\n", total_div, clock_div, wrap);    
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clock_div);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_init(slice_num, &cfg, true);
    pwm_set_chan_level(slice_num, channel, wrap / 2); // 50%デューティサイクル
    
    printf("PWM設定: 表示周波数=%u Hz, 実際の周波数=%.2f Hz, clock_div=%.2f, wrap=%u\n", display_freq_hz, (float)sys_clock / ((wrap + 1) * clock_div), clock_div, wrap);
    // 表示用周波数を保存
    current_clk_freq = display_freq_hz;
}


// 通信専用のコア0,1で共有するUART通信のためのグローバル変数
static volatile uint8_t uart_tx_data = 0;   // 送信データバッファ(Z80からPicoへ: 1バイト)
static volatile uint8_t uart_rx_data = 0;   // 受信データバッファ(PicoからZ80へ: 1バイト)
static volatile bool    uart_tx_ready = true;   // 送信可フラグ (true=Ready, false=Busy)
static volatile bool    uart_rx_ready = false;  // 受信完了フラグ (true=Ready, false=Empty)

// Memory Maped I/O UART - UARTDR(DataReg.):1FF0H, UARTCR(Ctrl-Reg.):1FF1H b0=受信文字あり,b1=送信可
#define UARTDR 0xE000
#define UARTCR 0xE001

// コア1のエントリポイント - Z80バスエミュレーション、UARTメモリメモリマップド版
__attribute__((noinline)) void __time_critical_func(core1_entry)(void) {;
    uint count = 0;
    uint8_t data_byte = 0;
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();
    // GP0-29(RX0:GP0,TX0:GP1,D0-D7:GP2-9,A0-A12:GP10-22,MRWR#:GP25,MRRD#:GP26,A15:GP27,CLK:GP28)
    const uint32_t mrwr_mask = (1u << MRWR_PIN);
    const uint32_t a15_mask = (1u << A15_PIN);
    const uint32_t a14_mask = (1u << A14_PIN);
    const uint32_t a13_mask = (1u << A13_PIN);
    while (true) {
        uint32_t agpio = pio_sm_get_blocking(pio, sm_emu);          // GPIO0-29の値を取得
        uint32_t adrs_word = (agpio >> ADDR_PINS_BASE) & 0x1FFF;    // A0-A12の13ビットを抽出
        if (agpio & a15_mask) adrs_word |= 0x8000;
        if (agpio & a14_mask) adrs_word |= 0x4000;
        if (agpio & a13_mask) adrs_word |= 0x2000;
        if (!(agpio & mrwr_mask)) {   //  MRWR=0 メモリーライト
            data_byte = (uint8_t)(agpio >> DATA_PINS_BASE); // D0-D7の8ビットを抽出
            if (adrs_word == UARTDR) {      // UART データレジスタ
                uart_tx_data = data_byte;   // UART送信バッファに書込み
                __dmb();                    // データメモリバリア
                uart_tx_ready = false; // 書き込まれたので送信Busy
            } else {
                memory[adrs_word] = data_byte;      // メモリーに書込み
            }
        } else { // MRWR=1 メモリーリード
            if (adrs_word == UARTDR) {  // UARTデータレジスタ
                data_byte = uart_rx_data;       // UART受信データ
                __dmb();                        // データメモリバリア
                uart_rx_ready = false;          // 読み取ったのでフラグを下ろす
            } else if (adrs_word == UARTCR) {   // UARTコントロールレジスタ
                data_byte = 0;
                if (uart_rx_ready) data_byte |= 0x01;   //  Bit0: RX Ready, Bit1: TX Ready
                if (uart_tx_ready) data_byte |= 0x02;
            } else {
                data_byte = memory[adrs_word];      // メモリーから読出し
            }
            pio_sm_put(pio, sm_emu, data_byte);     // Z80データバスに出力
        } 
        count++;
        if (false) { // デバッグ用 Z80_freq = 20  (20Hz) で使用する
            printf("%05u BUS:%08X ADRS:%04X DATA:%02X  A15:%d MRRD:%d MRWR:%d\n",
                count, agpio, adrs_word, (uint)data_byte, //);
                (agpio >> A15_PIN) & 1, (agpio >> MRRD_PIN) & 1, (agpio >> MRWR_PIN) & 1
            );
        }
    }
}

// --- UART Task (Core 0) ---
void UART_task(void) {
  printf("task UART start..\n");
  while (true) {
    // 送信処理 (Z80 -> USB)
        // 送信処理: Z80がデータを書き込んで Busy になったら実行
        if (!uart_tx_ready) {
            if (tud_cdc_connected() && tud_cdc_write_available() > 0) {
                putchar(uart_tx_data);
                __dmb();
                uart_tx_ready = true; // 送信完了（readyに戻す）
            }
        }
        // 受信処理(US->Z80) RX Readyがfalseの場合のみ入力をチェック
        if (!uart_rx_ready) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                uart_rx_data = (uint8_t)c;
                __dmb();
                uart_rx_ready = true;
            }
        }
        sleep_ms(1);
    }
}


//
// メインルーチン
//
__attribute__((noinline)) int __time_critical_func(main)(void) {
//   uint32_t sysclk = 360 * 1000;           // Pico2 システムクロック 280/320/360MHz 
   uint32_t sysclk = 380 * 1000;           // Pico2 システムクロック 280/320/360MHz 
//    uint32_t sysclk = 150 * 1000;           // Pico2 システムクロック 150MHz 
    vreg_set_voltage(VREG_VOLTAGE_1_30);    // 電圧を1.3Vに設定
    sleep_ms(100);                          // 電圧安定のための待機
    set_sys_clock_khz(sysclk, true);        // 高速動作
    set_qspi_clock_divider(sysclk, 133000); // QSPIクロックを133MHz以下に

    stdio_init_all();
 //   setbuf(stdout, NULL);           // 標準出力のバッファリングを無効化 

//    // UART0の初期化
//    uart_init(UART_ID, BAUD_RATE);
//    // UARTピンの設定（GPIO0=TX, GPIO1=RX）
//    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
//    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // PIO初期化
    // DATA-BUS GP2-9：入出力
    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio, DATA_PINS_BASE + i);
    }
    // ADDR-BUS GP10-22：入力(13ピン A0-A12)
    for (int i = 0; i < 13; i++) {
        pio_gpio_init(pio, ADDR_PINS_BASE + i);
    }
    pio_gpio_init(pio, A13_PIN); //  A13 (GP0:入力)の初期化
    pio_gpio_init(pio, A14_PIN); //  A14 (GP1:入力)の初期化
    pio_gpio_init(pio, A15_PIN); //  A15 (GP27:入力)の初期化

    pio_gpio_init(pio, MRWR_PIN); // MRWR#　メモリライト (GP25:入力) の初期化
    pio_gpio_init(pio, MRRD_PIN); // MRRD#　メモリリード (GP26:入力) の初期化

    // SM0: trg_rw (Detection of falling edge on MRRD#/MRWR#
    uint offset_trg = pio_add_program(pio, &trg_rw_program);
    pio_sm_config c_trg = trg_rw_program_get_default_config(offset_trg);
//    sm_config_set_jmp_pin(&c_trg, MRRD_PIN); //  MRRD# (GP26）をJMPピンとして設定
    sm_config_set_in_pins(&c_trg, MRWR_PIN); // base = GP25 (MRWR#, GP26 (MRRD#))
//    sm_config_set_in_pins(&c_trg, MRRD_PIN); // base = GP26 (MRRD#)
    pio_sm_init(pio, sm_trg, offset_trg, &c_trg);
//    pio_sm_set_enabled(pio, sm_trg, true);


    uint offset = pio_add_program(pio, &m_emu_program);
    pio_sm_config c = m_emu_program_get_default_config(offset);
    sm_config_set_in_pins(&c, 0);   // GPIO0-29全てを入力として設定
    sm_config_set_out_pins(&c, DATA_PINS_BASE, 8);
    sm_config_set_jmp_pin(&c, MRRD_PIN); //  MRRD# (GP26）をJMPピンとして設定
    pio_sm_set_consecutive_pindirs(pio, sm_emu, DATA_PINS_BASE, 8, false); // D0-7ピン初期化(入力)

    // シフトレジスタの設定(Auto push 30bit/pull 8bit)
    sm_config_set_in_shift(&c, false, true, 30); // ISR（入力シフトレジスタ）のシフト方向
    sm_config_set_out_shift(&c, true, true, 8); // OSR（出力シフトレジスタ）のシフト方向

    // sm のROMエミュプログラムを有効にする
    pio_sm_init(pio, sm_emu, offset, &c);
//    pio_sm_set_enabled(pio, sm_emu, true);

    sleep_ms(1); // 1ms待機

    init_rom_basic_code(); // rom_basic_const.cから初期化
    sleep_ms(3000); // 3秒待機

    // PWM TMPZ84C015 クロック出力初期化 (10MHz)
    printf("クロック出力(PWM) - 初期化中...\n"); 
//    init_clk_pwm(10000000);     // 10MHz
    init_clk_pwm(9000000);     // 8MHz
//    init_clk_pwm(8000000);     // 8MHz
//    init_clk_pwm(5000000);     // 5MHz
//      init_clk_pwm(2500000);     // 2.5MHz
//   init_clk_pwm(50);     // 50Hz
//   init_clk_pwm(20);     // 20Hz

// [Enter]入力を待つ
    printf("\n[Enter] を押すとPico2 RAMエミュレータのテスト開始します...\n");
    while (true) {
        int c = getchar_timeout_us(100000); // 100msタイムアウト
        if (c == '\r') { // [Enter]（CR）が入力されたら開始
            printf("Pico2 RAMエミュレータのテスト開始...\n");
            break;
        }
    }
    printf("\nPico2 システムクロック - %dMHz\n", sysclk / 1000);
//    printf("Pico2 - AKI-80 UART通信速度 - %d bps\n", BAUD_RATE);
    printf("Pico2 - AKI-80 UART通信(メモリマップド UARTDR:%04X, UARTCR:%04X)\n", UARTDR, UARTCR);
//    printf("リセット出力状態 - ON\n");
    if (current_clk_freq >= 1000000) {
        printf("AKI-80クロック出力(PWM-%.2fMHz) - ON\n", current_clk_freq / 1000000.0f);
    } else if (current_clk_freq >= 1000) {
        printf("クロック出力(PWM-%.2fKHz) - ON\n", current_clk_freq / 1000);
    } else {
        printf("AKI-80クロック出力(PWM-%dHz) - ON\n", current_clk_freq);
    }
    printf("RAMエミュレータ起動 - core1\n");
    multicore_launch_core1(core1_entry);
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE)
        printf("うーん、コア0では正しくありません!\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("コア0ではすべてうまくいきました!\n");
    }

// Z80BUS エミュレータ(PIO)を有効にする
    pio_sm_set_enabled(pio, sm_trg, true);
    pio_sm_set_enabled(pio, sm_emu, true);

    //    uint32_t reset_delay_ms = 1000;
//    printf("リセット解除まで - %d ms\n", reset_delay_ms);
//    add_alarm_in_ms(reset_delay_ms, reset_timer_callback, NULL, false);

    // メインループ
    printf("UART-USBブリッジ動作開始...\n");
    printf("リセットを解除してください\n");
    UART_task();

//    while (true) {
//        // UARTの受信データがあるかチェック
//        if (uart_is_readable(UART_ID)) {
//            int c = uart_getc(UART_ID);     // UARTから1文字受信
//            putchar_raw(c);                 // 受信データをそのままUSBへ送信
//        }
//        // USBから受信データがあるかチェック
//        int c;
//        if ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
//            uart_putc_raw(UART_ID, c); // UARTへデータ送信
//        }
//    }
    return 0;
}
