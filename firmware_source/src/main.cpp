#include <Arduino.h>
#include "config_pins.h"
#include "config_manager.h"
#include "hud_data.h"
#include "ble_vietmap_server.h"
#include "display_ui.h"

SystemState g_sysState;
uint32_t g_lastUiUpdate = 0;
uint32_t g_lastTimeUpdate = 0;
uint8_t  g_curMinute = 0;
uint8_t  g_curHour = 12;

void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println("\n=============================================");
    Serial.printf("   VIETMAP HUD PRO (NO-OBD) - %s\n", BOARD_NAME);
    Serial.println("=============================================");

    // 1. Khởi tạo Nút bấm
    #if BUTTON_PIN >= 0
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    #endif

    // 2. Khởi tạo Còi Buzzer
    #if BUZZER_PIN >= 0
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    #endif

    // 3. Khởi tạo LED RGB nếu có
    #if LED_RED_PIN >= 0
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN, HIGH);   // Tắt LED (Active Low)
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_BLUE_PIN, HIGH);
    #endif

    // 4. Khởi tạo Filesystem & Cấu hình
    ConfigManager::getInstance().begin();

    // 5. Khởi tạo Màn hình TFT
    DisplayUI::getInstance().begin();

    // 6. Khởi tạo BLE Server VIETMAP_HUD (nhẹ, tối ưu)
    BleVietmapServer::getInstance().begin(&g_sysState);

    // Kêu bíp báo hiệu sẵn sàng
    DisplayUI::getInstance().beep(2000, 150);
    Serial.println("[System] Ready for navigation!");
}

void loop() {
    uint32_t now = millis();

    // Cập nhật đồng hồ giả lập nội bộ (hoặc từ điện thoại)
    if (now - g_lastTimeUpdate > 60000) {
        g_lastTimeUpdate = now;
        g_curMinute++;
        if (g_curMinute >= 60) {
            g_curMinute = 0;
            g_curHour = (g_curHour + 1) % 24;
        }
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", g_curHour, g_curMinute);
        g_sysState.current_time_str = String(timeBuf);
    }

    // Cập nhật giao diện màn hình (10 FPS)
    if (now - g_lastUiUpdate >= 100) {
        g_lastUiUpdate = now;
        DisplayUI::getInstance().update(g_sysState);
    }

    // Xử lý nút bấm (Bấm ngắn: Đổi mode / Bấm giữ: Xoay màn hình)
    #if BUTTON_PIN >= 0
    static uint32_t pressStart = 0;
    static bool isPressed = false;
    bool btnState = (digitalRead(BUTTON_PIN) == LOW);

    if (btnState && !isPressed) {
        isPressed = true;
        pressStart = now;
    } else if (!btnState && isPressed) {
        isPressed = false;
        uint32_t pressDur = now - pressStart;
        if (pressDur > 1500) {
            // Bấm giữ > 1.5 giây: Xoay màn hình
            Serial.println("[Button] Long press: Rotate Screen");
            DisplayUI::getInstance().rotateScreen();
        } else if (pressDur > 50) {
            // Bấm ngắn: Đổi chế độ hiển thị
            Serial.println("[Button] Short press: Toggle Mode");
            DisplayUI::getInstance().toggleScreenMode();
        }
    }
    #endif

    delay(10);
}
