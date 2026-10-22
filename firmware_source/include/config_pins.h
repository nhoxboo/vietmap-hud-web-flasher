#pragma once
#include <Arduino.h>

// =============================================================================
// ĐỊNH NGHĨA PHẦN CỨNG CHO 2 DÒNG THIẾT BỊ (ĐÃ BỎ CÒI BÍP & CẢM BIẾN ÁNH SÁNG)
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

    // Chân SPI Màn hình ILI9341
    #define TFT_MOSI                13
    #define TFT_MISO                12
    #define TFT_SCLK                14
    #define TFT_CS                  15
    #define TFT_DC                  2
    #define TFT_RST                 -1
    #define TFT_BL                  21      // Đèn nền Backlight (Khóa 100% cực đại)

    // Ngoại vi
    #define LDR_SENSOR_PIN          -1      // Đã loại bỏ cảm biến quang trở LDR
    #define BUZZER_PIN              -1      // Đã loại bỏ còi chíp buzzer
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
    #define SCREEN_DEFAULT_ROTATION 0

    // Chân SPI Màn hình GC9A01
    #define TFT_MOSI                13
    #define TFT_MISO                -1
    #define TFT_SCLK                14
    #define TFT_CS                  15
    #define TFT_DC                  2
    #define TFT_RST                 -1
    #define TFT_BL                  21      // Đèn nền Backlight (Khóa 100% cực đại)

    // Ngoại vi
    #define LDR_SENSOR_PIN          -1      // Đã loại bỏ
    #define BUZZER_PIN              -1      // Đã loại bỏ
    #define LED_RED_PIN             -1
    #define LED_GREEN_PIN           -1
    #define LED_BLUE_PIN            -1
    #define BUTTON_PIN              0       // Nút Boot

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
