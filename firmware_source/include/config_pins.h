#pragma once
#include <Arduino.h>

// =============================================================================
// ĐỊNH NGHĨA PHẦN CỨNG CHO 2 DÒNG THIẾT BỊ POPULAR
// =============================================================================

#if defined(BOARD_ESP32_2432S024)
    // -------------------------------------------------------------------------
    // 1. ESP32-2432S024 (CYD - Cheap Yellow Display 2.4" / 2.8" ILI9341 320x240)
    // -------------------------------------------------------------------------
    #define BOARD_NAME              "ESP32-2432S024"
    #define SCREEN_IS_ROUND         false
    #define SCREEN_WIDTH            320
    #define SCREEN_HEIGHT           240
    #define SCREEN_DEFAULT_ROTATION 1       // Landscape ngang

    // Chân SPI Màn hình ILI9341 (Hardware HSPI / VSPI)
    #define TFT_MOSI                13
    #define TFT_MISO                12
    #define TFT_SCLK                14
    #define TFT_CS                  15
    #define TFT_DC                  2
    #define TFT_RST                 -1      // Nối chân Reset chip ESP32
    #define TFT_BL                  21      // Đèn nền màn hình (Backlight PWM)

    // Cảm biến quang trở ánh sáng tích hợp sẵn trên board CYD
    #define LDR_SENSOR_PIN          34      // ADC1_CH6 đo độ sáng môi trường

    // Còi chíp & Đèn LED tích hợp
    #define BUZZER_PIN              26      // Còi buzzer gắn sẵn
    #define LED_RED_PIN             4
    #define LED_GREEN_PIN           16
    #define LED_BLUE_PIN            17

    #define BUTTON_PIN              0       // Nút Boot GPIO 0

#elif defined(BOARD_ESP32_2424S012)
    // -------------------------------------------------------------------------
    // 2. ESP32-2424S012 (ESP32 Màn hình tròn 1.28" GC9A01 240x240 Round)
    // -------------------------------------------------------------------------
    #define BOARD_NAME              "ESP32-2424S012"
    #define SCREEN_IS_ROUND         true
    #define SCREEN_WIDTH            240
    #define SCREEN_HEIGHT           240
    #define SCREEN_DEFAULT_ROTATION 0       // Tròn đối xứng

    // Chân SPI Màn hình GC9A01
    #define TFT_MOSI                13
    #define TFT_MISO                -1
    #define TFT_SCLK                14
    #define TFT_CS                  15
    #define TFT_DC                  2
    #define TFT_RST                 -1
    #define TFT_BL                  21

    #define LDR_SENSOR_PIN          -1      // Không có sẵn quang trở
    #define BUZZER_PIN              25      // Chân xuất còi rời (nếu hàn thêm)
    #define LED_RED_PIN             -1
    #define LED_GREEN_PIN           -1
    #define LED_BLUE_PIN            -1

    #define BUTTON_PIN              0       // Nút Boot / Touch

#else
    #error "Vui long chon BOARD_ESP32_2432S024 hoac BOARD_ESP32_2424S012"
#endif

// =============================================================================
// THÔNG TIN BLUETOOTH BLE VIETMAP HUD
// =============================================================================
#define VIETMAP_DEVICE_NAME        "VIETMAP_HUD"
#define MAPVIET_DEVICE_NAME        "MAPVIET"

#define VIETMAP_SERVICE_UUID       "8a7e0001-4d6e-4c48-9a9d-484c504c0001"
#define VIETMAP_CHAR_WRITE_UUID    "8a7e0002-4d6e-4c48-9a9d-484c504c0001"
#define VIETMAP_CHAR_NOTIFY_UUID   "8a7e0003-4d6e-4c48-9a9d-484c504c0001"
