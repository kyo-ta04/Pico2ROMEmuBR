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

#include "ram_emu.pio.h"
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

#define FLAG_VALUE 123

// 65536 = 0-0xFFFF
#define MEMORY_SIZE 65536

// 現在設定されているクロック周波数
uint32_t current_clk_freq = 0;

// リセット出力関連
volatile bool reset_released = false;

static uint8_t test1_data[] = { // test program for debug
 0x3E, 0x00,       // LD A, 0x00
 0x32, 0x10, 0x00, // LD (0x0010), A
 0x3A, 0x10, 0x00, // LD A, (0x0010)
 0x3C,             // INC A
 0xC3, 0x02, 0x00  // JP 0x0002
};

static uint8_t test2_data[] = { // test program for debug
 0x3E, 0x41,       // LD A, 0x41
 0x32, 0x00, 0x1F, // LD (0x1F00), A
 0x76               // HALT
};


// Super AKI-80(TMPZ84C015) メモリ(RAM)エミュレーション用
static uint8_t __attribute__((aligned(4))) memory[MEMORY_SIZE];


// rom_basic[]をrom_data[]にコピーする初期化ルーチン
void init_rom_basic_code(void) {
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

static uint clk_pwm_slice_num;
static uint clk_pwm_channel;
static bool clk_pwm_initialized = false;

void clk_pwm_init(void) {
    clk_pwm_slice_num = pwm_gpio_to_slice_num(CLKOUT_PIN);
    clk_pwm_channel = pwm_gpio_to_channel(CLKOUT_PIN);
    gpio_set_function(CLKOUT_PIN, GPIO_FUNC_PWM);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 1.0f);
    pwm_config_set_wrap(&cfg, 0);
    pwm_init(clk_pwm_slice_num, &cfg, false);
    pwm_set_chan_level(clk_pwm_slice_num, clk_pwm_channel, 0);

    clk_pwm_initialized = true;
}

void clk_pwm_set_frequency(uint32_t display_freq_hz) {
    if (!clk_pwm_initialized) {
        clk_pwm_init();
    }

    uint32_t actual_freq_hz = display_freq_hz * 2;
    uint32_t sys_clock = clock_get_hz(clk_sys);

    float total_div = (float)sys_clock / actual_freq_hz;
    float clock_div = 1.0f;
    uint32_t temp = (total_div / clock_div) - 1;

    if (temp > 65535) {
        clock_div = total_div / 65536.0f;
        if (clock_div > 256.0f) {
            clock_div = 256.0f;
            temp = (total_div / clock_div) - 1;
            if (temp > 65535) {
                temp = 65535;
            }
        } else {
            temp = 65535;
        }
    }

    uint16_t wrap = (uint16_t)temp;
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clock_div);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_init(clk_pwm_slice_num, &cfg, false);
    pwm_set_chan_level(clk_pwm_slice_num, clk_pwm_channel, wrap / 2);

    current_clk_freq = display_freq_hz;
}

void clk_pwm_output_on(void) {
    if (!clk_pwm_initialized) {
        clk_pwm_init();
    }
    pwm_set_enabled(clk_pwm_slice_num, true);
}

void clk_pwm_output_off(void) {
    if (clk_pwm_initialized) {
        pwm_set_enabled(clk_pwm_slice_num, false);
    }
}

void init_clk_pwm(uint32_t display_freq_hz) {
    clk_pwm_init();
    clk_pwm_set_frequency(display_freq_hz);
    clk_pwm_output_on();
}


// PIO0 とステートマシンの定義
PIO pio_0 = pio0;
uint sm_emu = 0;
uint sm_pindir_L = 1;
uint sm_pindir_H = 2;

PIO pio_1 = pio1;
uint sm_trg_r = 0;
uint sm_trg_w = 1;


// 通信専用のコア0,1で共有するUART通信のためのグローバル変数
static volatile uint8_t __attribute__((section(".scratch_x"))) uart_tx_data = 0;   // 送信データバッファ(Z80からPicoへ: 1バイト)
static volatile uint8_t __attribute__((section(".scratch_x"))) uart_rx_data = 0;   // 受信データバッファ(PicoからZ80へ: 1バイト)
static volatile bool    __attribute__((section(".scratch_x"))) uart_tx_ready = true;   // 送信可フラグ (true=Ready, false=Busy)
static volatile bool    __attribute__((section(".scratch_x"))) uart_rx_ready = false;  // 受信完了フラグ (true=Ready, false=Empty)

// Memory Maped I/O UART - UARTDR(DataReg.):E000H, UARTCR(Ctrl-Reg.):E001H b0=受信文字あり,b1=送信可
#define UARTDR 0xE000
#define UARTCR 0xE001

// コア1のエントリポイント - Z80バスエミュレーション、UARTメモリメモリマップド版
__attribute__((noinline)) void __time_critical_func(core1_entry)(void) {;
    uint32_t count = 0;
    uint flg = 0;
    uint8_t data_byte = 0;
    uint NumOfLogTimes = 30; // デバッグ用、ログを表示する行数
    uint LogCount = 0;

    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();
    // GP0-29(A13:GP0,A14:GP1,D0-D7:GP2-9,A0-A12:GP10-22,MRWR#:GP25,MRRD#:GP26,A15:GP27,CLK:GP28)
    const uint32_t mrwr_mask = (1u << MRWR_PIN);
    while (true) {
        uint32_t agpio = pio_sm_get_blocking(pio_0, sm_emu);          // GPIO0-29の値を取得
        // === 最速アドレス計算（branchless + 最小演算）===
        // A0-A12: agpio[22:10], A13-A14: agpio[1:0], A15: agpio[27] 
        // 27bit目を15bit目に持ってくるため右に12シフトし、0x8000でマスク
        uint32_t adrs_word = ((agpio >> ADDR_PINS_BASE) & 0x1FFFu) | 
                             ((agpio & 0x3u) << 13) | 
                             ((agpio >> 12) & 0x8000u);
        // UART領域(0xE000, 0xE001)かどうかの判定をマスク(0xFFFE)で一括で行う
        // __builtin_expect(..., 0) でコンパイラに「基本はfalse(通常メモリ)である」と伝える
        if (__builtin_expect((adrs_word & 0xFFFEu) == 0xE000u, 0)) {
            data_byte = (uint8_t)(agpio >> DATA_PINS_BASE);
            // --- UARTアクセス ---
            if (!(agpio & mrwr_mask)) {   // メモリーライト
                if (adrs_word == UARTDR) { // UARTDRへの書き込み(送信)
                    uart_tx_data = data_byte;
                    uart_tx_ready = false;
                }
            } else {                      // メモリーリード
                if (adrs_word == UARTDR) {  // UARTDRの読み込み(受信)
                    data_byte = uart_rx_data;
                    uart_rx_ready = false;
                } else { // UARTCR ステータスレジスタの読み込み
                    data_byte = (uint8_t)uart_rx_ready | ((uint8_t)uart_tx_ready << 1);
                }
                pio_sm_put(pio_0, sm_emu, data_byte);
            }
        } else {
            // --- 通常RAM/ROMアクセス (HOT PATH: 最速で処理する) ---
            if (!(agpio & mrwr_mask)) {   // メモリーライト
                data_byte = (uint8_t)(agpio >> DATA_PINS_BASE);
                    memory[adrs_word] = data_byte;
            } else {                      // メモリーリード
                data_byte = memory[adrs_word];
                pio_sm_put(pio_0, sm_emu, data_byte);
            }
        }
#if 0
        count++; // デバッグ用 Z80_freq = 20-50  (20-50Hz) で使用する
        if (flg) {
            printf("%10u BUS:%08X ADRS:%04X DATA:%02X  A15:%d MRRD:%d MRWR:%d\n",
            count, agpio, adrs_word, (uint)data_byte, 
            (agpio >> A15_PIN) & 1, (agpio >> MRRD_PIN) & 1, (agpio >> MRWR_PIN) & 1);
//            if (LogCount == 0) {
//                clk_pwm_set_frequency(50);     // デバッグ用、クロックを50Hzにして動きを遅くする
//                clk_pwm_output_on();    // クロック出力を再開
//            }
//            if (++LogCount >= NumOfLogTimes) {
//                clk_pwm_output_off();    // クロック出力を停止
//                clk_pwm_set_frequency(2500000);     // デバッグ用、クロックを2.5MHzに戻す
//                LogCount = 0;
//                flg = 0;
//                clk_pwm_output_on();    // クロック出力を再開
//            }
        }
#endif
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
#if 1
                putchar(uart_tx_data);
#else
                if (uart_tx_data < ' ' || uart_tx_data >= 0x7F) {
                    putchar('.'); // 制御文字はドットで表示
                    if (uart_tx_data == '\n') {
                        putchar('\n');
                    }
                } else {
                  putchar(uart_tx_data);
                }
                printf("[%02X]\n",uart_tx_data); // デバッグ用送信データ表示
#endif
                // __dmb();
                uart_tx_ready = true; // 送信完了（readyに戻す）
            }
        }
        // 受信処理(US->Z80) RX Readyがfalseの場合のみ入力をチェック
        if (!uart_rx_ready) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                uart_rx_data = (uint8_t)c;
                // __dmb();
                uart_rx_ready = true;
            }
        }
        sleep_us(500); // 500us待機（CPU負荷を下げるため）
    }
}

//
// メインルーチン
//
__attribute__((noinline)) int __time_critical_func(main)(void) {
    uint32_t sysclk = clock_get_hz(clk_sys);
    int sysvolt = VREG_VOLTAGE_1_30;
    // int sysvolt = VREG_VOLTAGE_1_15;
    // int sysvolt = VREG_VOLTAGE_1_10;

    sysclk = 360 * 1000;                    // Pico2 システムクロック 280/320/360MHz 
    vreg_set_voltage(sysvolt);              // コア電圧を設定
    sleep_ms(100);                          // 電圧安定のための待機
    set_sys_clock_khz(sysclk, true);        // 高速動作
    set_qspi_clock_divider(sysclk, 133000); // QSPIクロックを133MHz以下に

    stdio_init_all();
    //setbuf(stdout, NULL);           // 標準出力のバッファリングを無効化 

    // PIO初期化
    // DATA-BUS GP2-9：入出力
    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio_0, DATA_PINS_BASE + i);
    }
    // ADDR-BUS GP10-22：入力(13ピン A0-A12)
    for (int i = 0; i < 13; i++) {
        pio_gpio_init(pio_0, ADDR_PINS_BASE + i);
    }
    pio_gpio_init(pio_0, A13_PIN); //  A13 (GP0:入力)の初期化
    pio_gpio_init(pio_0, A14_PIN); //  A14 (GP1:入力)の初期化
    pio_gpio_init(pio_0, A15_PIN); //  A15 (GP27:入力)の初期化
    pio_gpio_init(pio_0, MRWR_PIN); // MRWR#　メモリライト (GP25:入力) の初期化
    pio_gpio_init(pio_0, MRRD_PIN); // MRRD#　メモリリード (GP26:入力) の初期化

    // PIO0 SM0: ROMエミュレーションプログラム
    uint offset = pio_add_program(pio_0, &m_emu_program);
    pio_sm_config c = m_emu_program_get_default_config(offset);
    sm_config_set_in_pins(&c, 0);   // GPIO0-29全てを入力として設定
    sm_config_set_out_pins(&c, DATA_PINS_BASE, 8);
    sm_config_set_jmp_pin(&c, MRRD_PIN); //  MRRD# (GP26）をJMPピンとして設定
    pio_sm_set_consecutive_pindirs(pio_0, sm_emu, DATA_PINS_BASE, 8, false); // D0-7ピン初期化(入力)

    // シフトレジスタの設定(Auto push 30bit/pull 8bit)
    sm_config_set_in_shift(&c, false, true, 30); // ISR（入力シフトレジスタ）のシフト方向
    sm_config_set_out_shift(&c, true, true, 8); // OSR（出力シフトレジスタ）のシフト方向
    // sm のROMエミュプログラムを初期化する
    pio_sm_init(pio_0, sm_emu, offset, &c);

    // PIO0 SM1/2: データ(D0-D7)ピン方向制御プログラム
    uint offset_pindir = pio_add_program(pio_0, &d_pindirs_program);
    pio_sm_config c_pindir_L = d_pindirs_program_get_default_config(offset_pindir);
    pio_sm_config c_pindir_H = d_pindirs_program_get_default_config(offset_pindir);
    sm_config_set_set_pins(&c_pindir_L, DATA_PINS_BASE, 4); // D0-D3
    sm_config_set_set_pins(&c_pindir_H, DATA_PINS_BASE + 4, 4); // D4-D7
    pio_sm_init(pio_0, sm_pindir_L, offset_pindir, &c_pindir_L);
    pio_sm_init(pio_0, sm_pindir_H, offset_pindir, &c_pindir_H);

    pio_gpio_init(pio_1, MRRD_PIN); // MRRD#　メモリリード (GP26:入力) の初期化
    pio_gpio_init(pio_1, MRWR_PIN); // MRWR#　メモリライト (GP25:入力) の初期化

    // PIO1 SM0/1: trg_pin (Detection of falling edge on MRRD#/MRWR#)
    uint offset_trg = pio_add_program(pio_1, &trg_pin_program);
    pio_sm_config c_trg_w = trg_pin_program_get_default_config(offset_trg);
    pio_sm_config c_trg_r = trg_pin_program_get_default_config(offset_trg);
    sm_config_set_in_pins(&c_trg_r, MRWR_PIN); // base = GP25 (MRWR#)
    sm_config_set_in_pins(&c_trg_w, MRRD_PIN); // base = GP26 (MRRD#)
    pio_sm_init(pio_1, sm_trg_r, offset_trg, &c_trg_r);
    pio_sm_init(pio_1, sm_trg_w, offset_trg, &c_trg_w);

    sleep_ms(1); // 1ms待機

    init_rom_basic_code(); // rom_basic_const.cから初期化
    sleep_ms(3000); // 3秒待機

// [Enter]入力を待つ
    printf("\nSuper AKI-80のリセットを有効にして下さい\n");
    printf("[Enter] を押すとPico2 RAMエミュレータのテスト開始します...\n");
    while (true) {
        int c = getchar_timeout_us(100000); // 100msタイムアウト
        if (c == '\r') { // [Enter]（CR）が入力されたら開始
            printf("Pico2 RAMエミュレータ(64KB)のテスト開始...\n");
            break;
        }
    }
    // PWM TMPZ84C015 クロック出力初期化 (10MHz)
    printf("クロック出力(PWM) - 初期化中...\n"); 
//   init_clk_pwm(12000000);     // 12MHz
 //  init_clk_pwm(11000000);     // 11MHz
   init_clk_pwm(10000000);     // 10MHz
//   init_clk_pwm(9000000);     // 9MHz
//   init_clk_pwm(8000000);     // 8MHz
//   init_clk_pwm(5000000);     // 5MHz
//   init_clk_pwm(2500000);     // 2.5MHz
//   init_clk_pwm(1000000);       // 1MHz
//   init_clk_pwm(200000);     // 200kHz
//   init_clk_pwm(100000);     // 100kHz
//   init_clk_pwm(10000);     // 10kHz
//   init_clk_pwm(1000);     // 1kHz
//   init_clk_pwm(100);     // 100Hz
//   init_clk_pwm(50);     // 50Hz
//   init_clk_pwm(20);     // 20Hz
    float volt = 0;
    if (sysvolt == VREG_VOLTAGE_1_10)
        volt = 1.10;
    else if (sysvolt == VREG_VOLTAGE_1_15)
        volt = 1.15;
    else if (sysvolt == VREG_VOLTAGE_1_25)
        volt = 1.25;
    else if (sysvolt == VREG_VOLTAGE_1_30)
        volt = 1.30;

    //  エミュレーション開始(core1)
    printf("Pico2  Core:%0.2fV Clock:%uMHz\n", volt, sysclk / 1000);
    printf("Super AKI-80 UART通信(メモリマップド UARTDR:%04X, UARTCR:%04X)\n", UARTDR, UARTCR);
    printf("Super AKI-80クロック出力");
    if (current_clk_freq >= 1000000) {
        printf("(PWM-%.2fMHz) - ON\n", current_clk_freq / 1000000.0f);
    } else if (current_clk_freq >= 1000) {
        printf("(PWM-%.2fKHz) - ON\n", current_clk_freq / 1000);
    } else {
        printf("(PWM-%dHz) - ON\n", current_clk_freq);
    }
    printf("RAMエミュレータ起動 - core1\n");
    multicore_launch_core1(core1_entry);
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE)
        printf("うーん、コア1では正しくありません!\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("コア1ではすべてうまくいきました!\n");
    }

// Z80BUS エミュレータ(PIO)を有効にする
    pio_sm_set_enabled(pio_0, sm_emu, true);
    pio_sm_set_enabled(pio_1, sm_trg_w,true);
    pio_sm_set_enabled(pio_1, sm_trg_r,true);
    pio_sm_set_enabled(pio_0, sm_pindir_L,true);
    pio_sm_set_enabled(pio_0, sm_pindir_H,true);

    // メインループ
    printf("UART-USBブリッジ動作開始...\n");
    printf("リセットを解除してください\n");
    int d = 0;
    while ((d = getchar_timeout_us(100000)) != PICO_ERROR_TIMEOUT)
        printf("<%02X",d);
    UART_task();

    return 0;
}
